#include <stdexcept>
#include <vulkan/vk_static_buffer.h>
#include <vulkan/vk_structs.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

	void Static_Buffer::init(
		uint32_t initialize_size, uint32_t initialize_staging_size, VkBufferUsageFlags usage_flags,
		const std::vector<VkFence>& draw_fences, VkPhysicalDevice physical_device, VkDevice device
	) {
		this->device = device;
		this->draw_fences = draw_fences;
		inner_buffer.make_buffer(
			initialize_size, usage_flags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physical_device, device
		);
		staging_buffer.make_buffer(
			initialize_staging_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, physical_device, device, true
		);
	}

	void Static_Buffer::wait_synchronize_finish(uint32_t frame_index) {
		/**
		 * @Note: before anything happen we need to wait buffer resize success
		 * first to delete old buffer and continue to use it!
		 */
		if (resize_sync_info.is_resizing) {
			vkWaitForFences(device, draw_fences.size(), draw_fences.data(), VK_TRUE, UINT64_MAX);
			resize_sync_info.old_buffer.destroy();
			resize_sync_info.is_resizing = false;
		}
		if (copied_sync_info.is_copying) {
			vkWaitForFences(device, 1, &copied_sync_info.fence, VK_TRUE, UINT64_MAX);
			copied_sync_info.is_copying = false;
			available_staging_ranges = {};
			id_to_staging_ranges.clear();
			current_staging_offset = 0;
		}
		this->current_frame = current_frame;
	}

	uint32_t Static_Buffer::upload_data(void* data, uint32_t size, uint32_t element_stride) {
		element_stride = std::max((uint32_t)1, element_stride);
		uint32_t range_id = range_id_generator.gen_id();
		Buffer_Range using_range =
			Utils::find_suitable_buffer_range(available_ranges, current_offset, size, element_stride);
		size_required += using_range.total_size();
		Buffer_Range staging_using_range =
			Utils::find_suitable_buffer_range(available_staging_ranges, current_staging_offset, size);
		uint32_t staging_size_required = staging_using_range.offset + staging_using_range.total_size();
		if (staging_buffer.size < staging_size_required) {
			uint32_t new_staging_size = (uint32_t)(staging_size_required * 1.5f);
			auto buffer = staging_buffer.resize(new_staging_size);
		}
		staging_buffer.copy_data(size, data, staging_using_range.inner_offset());
		id_to_staging_ranges.insert(range_id, staging_using_range);
		return id_to_ranges.insert(range_id, using_range);
	}

	const Buffer_Range& Static_Buffer::view_slot_info(uint32_t id) {
		if (!id_to_ranges.has(id)) {
			throw std::runtime_error("Fail to view static range: " + std::to_string(id) + "!");
		}
		return id_to_ranges.get(id);
	}

	bool Static_Buffer::remove_data(uint32_t id) {
		if (!id_to_ranges.has(id)) {
			return false;
		}
		/**
		 * @Note: in case data is not send on GPU we just reuse
		 * range on staging and inner buffer.
		 */
		if (id_to_staging_ranges.has(id)) {
			Buffer_Range remove_range = id_to_staging_ranges.get(id);
			available_staging_ranges.push(remove_range);
			remove_range = id_to_ranges.get(id);
			available_ranges.push(remove_range);
			id_to_staging_ranges.erase(id);
			id_to_ranges.erase(id);
			range_id_generator.release_id(id);
			return true;
		}
		if (queue_remove_range.has(id)) {
			return false;
		}
		Resource_Synchronize_Information remove{};
		remove.release_frame(current_frame);
		queue_remove_range.insert(id, remove);
		return true;
	}

	void Static_Buffer::flush_removed_data(uint32_t frame_index) {
		/**
		 * @Note: delete and reuse resource check by all frame finish
		 * it's none block because flush_data call when last frame index finish
		 */
		range_removed_ids.clear();
		for (const uint32_t& id : queue_remove_range.keys()) {
			auto& remove = queue_remove_range.modify(id);
			remove.release_frame(frame_index);
			if (remove.can_remove()) {
				Buffer_Range range = id_to_ranges.get(id);
				available_ranges.push(range);
				id_to_ranges.erase(id);
				range_id_generator.release_id(id);
				range_removed_ids.push_back(id);
			}
		}
		for (const uint32_t& id : range_removed_ids) {
			queue_remove_range.erase(id);
		}
	}

	void Static_Buffer::flush_data(VkCommandBuffer command_buffer, uint32_t frame_index) {
		if (inner_buffer.size < size_required) {
			uint32_t size = (uint32_t)size_required * 1.5f;
			resize_sync_info.old_buffer = inner_buffer.record_resize(command_buffer, size);
			resize_sync_info.is_resizing = true;
		}
		if (id_to_staging_ranges.size() > 0) {
			Utils::record_buffer_barrier(
				command_buffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, inner_buffer.buffer, 0,
				inner_buffer.size
			);
			{
				copied_data.clear();
				for (const uint32_t id : id_to_staging_ranges.keys()) {
					Buffer_Range staging_range = id_to_staging_ranges.get(id);
					if (!id_to_ranges.has(id))
						continue;
					Buffer_Range static_range = id_to_ranges.get(id);
					copied_data.push_back(
						{(VkDeviceSize)staging_range.inner_offset(), (VkDeviceSize)static_range.inner_offset(),
						 (VkDeviceSize)staging_range.total_size()}
					);
				}
				vkCmdCopyBuffer(
					command_buffer, staging_buffer.buffer, inner_buffer.buffer, copied_data.size(), copied_data.data()
				);
			}
			Utils::record_buffer_barrier(
				command_buffer, VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, inner_buffer.buffer, 0, inner_buffer.size
			);
			copied_sync_info.is_copying = true;
			copied_sync_info.fence = draw_fences[frame_index];
		}
	}

	void Static_Buffer::clear() {
		available_ranges = {};
		available_staging_ranges = {};
		id_to_staging_ranges.clear();
		id_to_ranges.clear();
		range_id_generator.clear();
		size_required = 0;
		current_offset = 0;
	}

	void Static_Buffer::destroy() {
		staging_buffer.destroy();
		inner_buffer.destroy();
		if (resize_sync_info.is_resizing) {
			resize_sync_info.old_buffer.destroy();
		}
	}
} // namespace Vulkan