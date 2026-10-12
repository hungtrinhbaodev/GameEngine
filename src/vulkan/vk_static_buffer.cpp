#include <stdexcept>
#include <vulkan/vk_static_buffer.h>
#include <vulkan/vk_structs.h>

namespace Vulkan {

	void Remove_Data_Synchronize_Information::release_fence(VkFence fence) {
		for (int i = 0; i < wait_fences.size(); i++) {
			if (wait_fences[i] == fence) {
				wait_fences[i] = wait_fences.back();
				wait_fences.pop_back();
				return;
			}
		}
	}

	bool Remove_Data_Synchronize_Information::can_remove() {
		return wait_fences.size() <= 0;
	}

	void Static_Buffer::init(
		uint32_t initialize_size, uint32_t initialize_staging_size, VkBufferUsageFlags usage_flags,
		const std::vector<VkFence>& draw_fences, VkPhysicalDevice physical_device, VkDevice device
	) {
		this->device = device;
		this->draw_fences = draw_fences;
		staging_buffer.init(initialize_staging_size, physical_device, device);
		inner_buffer.make_buffer(
			initialize_size, usage_flags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physical_device, device
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
			staging_buffer.clear();
		}
	}

	uint32_t Static_Buffer::upload_data(void* data, uint32_t size, uint32_t element_stride) {
		element_stride = std::max((uint32_t)1, element_stride);
		uint32_t range_id = range_id_generator.gen_id();
		Static_Buffer_Range using_range{0, 0, 0};
		if (available_ranges.size() > 0 && available_ranges.top().is_suitable_with(size, element_stride)) {
			Static_Buffer_Range optimal_range = available_ranges.top();
			available_ranges.pop();
			Static_Buffer_Range remain_range = optimal_range.fit_with(size, element_stride);
			available_ranges.push(remain_range);
			using_range = optimal_range;
		} else {
			uint32_t align = element_stride - (current_offset % element_stride);
			using_range = {current_offset, align, size};
			current_offset += using_range.total_size();
		}
		size_required += using_range.total_size();
		uint32_t ssbo_id = staging_buffer.upload_data(data, size);
		ssbo_id_to_id.insert(ssbo_id, range_id);
		id_to_ssbo_id.insert(range_id, ssbo_id);
		return id_to_ranges.insert(range_id, using_range);
	}

	const Static_Buffer_Range& Static_Buffer::view_slot_info(uint32_t id) {
		if (!id_to_ranges.has(id)) {
			throw std::runtime_error("Fail to view static range: " + std::to_string(id) + "!");
		}
		return id_to_ranges.get(id);
	}

	bool Static_Buffer::remove_data(uint32_t id) {
		if (!id_to_ranges.has(id)) {
			return false;
		}
		if (queue_remove.has(id)) {
			return false;
		}
		queue_remove.insert(id, {draw_fences});
		return true;
	}

	void Static_Buffer::flush_removed_data(uint32_t frame_index) {
		/**
		 * @Note: delete and reuse resource check by all frame finish
		 * it none block because flush_data call when last frame index finish
		 */
		removed_ids.clear();
		for (const uint32_t& id : queue_remove.keys()) {
			auto& remove = queue_remove.modify(id);
			remove.release_fence(draw_fences[frame_index]);
			if (remove.can_remove()) {
				if (id_to_ssbo_id.has(id)) {
					uint32_t ssbo_id = id_to_ssbo_id.get(id);
					ssbo_id_to_id.erase(ssbo_id);
					id_to_ssbo_id.erase(id);
				}
				Static_Buffer_Range range = id_to_ranges.get(id);
				available_ranges.push(range);
				id_to_ranges.erase(id);
				range_id_generator.release_id(id);
				removed_ids.push_back(id);
			}
		}
		for (const uint32_t& id : removed_ids) {
			queue_remove.erase(id);
		}
	}

	void Static_Buffer::flush_data(VkCommandBuffer command_buffer, uint32_t frame_index) {
		if (inner_buffer.size < size_required) {
			uint32_t size = (uint32_t)size_required * 1.5f;
			resize_sync_info.old_buffer = inner_buffer.record_resize(command_buffer, size);
			resize_sync_info.is_resizing = true;
		}
		for (const uint32_t ssbo_id : ssbo_id_to_id.keys()) {
			uint32_t range_id = ssbo_id_to_id.get(ssbo_id);
			Static_Buffer_Range static_range = id_to_ranges.get(range_id);
			staging_buffer.transfer_data_to(ssbo_id, inner_buffer.buffer, static_range.inner_offset());
		}
		if (id_to_ssbo_id.size() > 0) {
			VkBufferMemoryBarrier start_transfer_barrier = Structs::make_buffer_barier(
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, inner_buffer.buffer, 0, inner_buffer.size
			);
			vkCmdPipelineBarrier(
				command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1,
				&start_transfer_barrier, 0, nullptr
			);
			staging_buffer.record_flush_transfer_data(command_buffer);
			VkBufferMemoryBarrier finish_transfer_barrier = Structs::make_buffer_barier(
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT,
				inner_buffer.buffer, 0, inner_buffer.size
			);
			vkCmdPipelineBarrier(
				command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1,
				&finish_transfer_barrier, 0, nullptr
			);
			id_to_ssbo_id.clear();
			ssbo_id_to_id.clear();
			copied_sync_info.is_copying = true;
			copied_sync_info.fence = draw_fences[frame_index];
		}
	}

	void Static_Buffer::clear() {
		id_to_ssbo_id.clear();
		ssbo_id_to_id.clear();
		staging_buffer.clear();
		id_to_ranges.clear();
		range_id_generator.clear();
		size_required = 0;
		current_offset = 0;
	}

	void Static_Buffer::destroy() {
		staging_buffer.destroy();
		inner_buffer.destroy();
	}
} // namespace Vulkan