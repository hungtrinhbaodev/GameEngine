#include <stdexcept>
#include <vulkan/vk_ssbo_buffer.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

	void SSBO_Buffer::init(uint32_t initialize_size, VkPhysicalDevice physical_device, VkDevice device) {
		this->device = device;
		inner_buffer.make_buffer(
			initialize_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, physical_device, device, true
		);
		for (int i = 0; i < storage_buffers.size(); i++) {
			storage_buffers[i].make_buffer(
				initialize_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				physical_device, device
			);
		}
	}

	void SSBO_Buffer::setup_frame(uint32_t current_frame) {
		this->current_frame = current_frame;
	}

	uint32_t SSBO_Buffer::upload_data(void* data, uint32_t size, uint32_t element_stride) {
		element_stride = std::max(element_stride, (uint32_t)1);
		uint32_t range_id = range_id_generator.gen_id();
		Buffer_Range using_range =
			Utils::find_suitable_buffer_range(available_ranges, current_offset, size, element_stride);
		uint32_t size_required = using_range.offset + using_range.total_size();
		if (inner_buffer.size < size_required) {
			uint32_t new_size = (uint32_t)(size_required * 1.5f);
			Remove_Buffer_Synchronize_Information remove_buffer_info{inner_buffer.resize(new_size, false), {}};
			queue_remove_inner_buffer.push_back(remove_buffer_info);
		}
		inner_buffer.copy_data(using_range.size, data, using_range.inner_offset());
		for (auto& dirty_ids : frame_dirty_ids) {
			dirty_ids.insert(range_id, true);
		}
		return ranges_by_id.insert(range_id, using_range);
	}

	void SSBO_Buffer::update_data(uint32_t id, void* data, uint32_t offset, uint32_t size) {
		Buffer_Range range = view_slot(id);
		if (size <= 0) {
			size = range.inner_size();
		}
		if (offset <= 0) {
			offset = range.inner_offset();
		}
		inner_buffer.copy_data(size, data, offset);
		for (auto& dirty_ids : frame_dirty_ids) {
			dirty_ids.insert(id, true);
		}
	}

	void SSBO_Buffer::remove_data(uint32_t id) {
		if (!ranges_by_id.has(id))
			return;
		if (queue_remove_range_inner_buffer.has(id))
			return;
		Resource_Synchronize_Information remove_info{};
		remove_info.release_frame(current_frame);
		queue_remove_range_inner_buffer.insert(id, remove_info);
	}

	Buffer_Range SSBO_Buffer::view_slot(uint32_t id) {
		if (!ranges_by_id.has(id)) {
			throw std::runtime_error(
				"Fail to view SSBO buffer range with id: " + std::to_string(id) + " please check!"
			);
		}
		return ranges_by_id.get(id);
	}

	void SSBO_Buffer::transfer_data_to(uint32_t id, VkBuffer dst_buffer, uint32_t dst_offset) {
		Buffer_Range range = view_slot(id);
		Transfer_Data_Information transfer_data{dst_offset, range.offset, range.size, dst_buffer};
		queue_transfer.push_back(transfer_data);
	}

	void SSBO_Buffer::flush_removed_data(uint32_t frame_index) {
		/**
		 * @Note: remove old buffer in queue first.
		 */
		for (int i = 0; i < queue_remove_inner_buffer.size(); i++) {
			Remove_Buffer_Synchronize_Information& remove_info = queue_remove_inner_buffer[i];
			remove_info.sync_info.release_frame(frame_index);
			if (remove_info.sync_info.can_remove()) {
				remove_info.remove_buffer.destroy();
				queue_remove_inner_buffer[i] = queue_remove_inner_buffer.back();
				queue_remove_inner_buffer.pop_back();
				i--;
			}
		}
		range_remove_ids.clear();
		for (const uint32_t id : queue_remove_range_inner_buffer.keys()) {
			auto& remove = queue_remove_range_inner_buffer.modify(id);
			remove.release_frame(frame_index);
			if (remove.can_remove()) {
				const Buffer_Range& range = ranges_by_id.get(id);
				available_ranges.push(range);
				range_id_generator.release_id(id);
				ranges_by_id.erase(id);
				range_remove_ids.push_back(id);
			}
		}
		for (const uint32_t& id : range_remove_ids) {
			queue_remove_range_inner_buffer.erase(id);
		}
	}

	void SSBO_Buffer::flush_data_to_storage_buffer(VkCommandBuffer command_buffer, uint32_t current_frame) {
		Resize_Buffer_Synchronize_Information& resize_info = frame_resize_storage_buffer_info[current_frame];
		if (resize_info.is_resizing) {
			resize_info.old_buffer.destroy();
			resize_info.is_resizing = false;
		}
		Buffer& storage_buffer = storage_buffers[current_frame];
		if (storage_buffer.size < inner_buffer.size) {
			resize_info.old_buffer = storage_buffer.record_resize(command_buffer, inner_buffer.size);
			resize_info.is_resizing = true;
		}
		copy_ranges.clear();
		Sparse_Set<bool>& dirty_ids = frame_dirty_ids[current_frame];
		for (const uint32_t& id : dirty_ids.keys()) {
			Buffer_Range range = view_slot(id);
			copy_ranges.push_back({range.inner_offset(), range.inner_offset(), range.inner_size()});
		}
		dirty_ids.clear();
		if (copy_ranges.size() <= 0) {
			return;
		}
		Utils::record_buffer_barrier(
			command_buffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, storage_buffer.buffer, 0, storage_buffer.size
		);
		{
			vkCmdCopyBuffer(
				command_buffer, inner_buffer.buffer, storage_buffer.buffer, copy_ranges.size(), copy_ranges.data()
			);
		}
		Utils::record_buffer_barrier(
			command_buffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, storage_buffer.buffer, 0, storage_buffer.size
		);
	}

	std::map<VkBuffer, std::vector<VkBufferCopy>> SSBO_Buffer::make_copied_data() {
		std::map<VkBuffer, std::vector<VkBufferCopy>> copied_data;
		for (const Transfer_Data_Information& transfer : queue_transfer) {
			VkBuffer dst_buffer = transfer.dst_buffer;
			if (copied_data.find(dst_buffer) == copied_data.end()) {
				copied_data[dst_buffer] = {};
			}
			VkBufferCopy region{
				(VkDeviceSize)(transfer.src_offset), (VkDeviceSize)(transfer.dst_offset),
				(VkDeviceSize)(transfer.size_transfer)
			};
			copied_data[dst_buffer].push_back(region);
		}
		return copied_data;
	}

	void SSBO_Buffer::flush_transfer_data() {
		std::map<VkBuffer, std::vector<VkBufferCopy>> copied_data = make_copied_data();
		if (copied_data.size() <= 0) {
			return;
		}
		Utils::copy_data_to_multi_buffer(inner_buffer.buffer, copied_data, device);
		queue_transfer.clear();
	}

	void SSBO_Buffer::record_flush_transfer_data(VkCommandBuffer command_buffer) {
		std::map<VkBuffer, std::vector<VkBufferCopy>> copied_data = make_copied_data();
		if (copied_data.size() <= 0) {
			return;
		}
		Utils::record_copy_data_to_multi_buffer(command_buffer, inner_buffer.buffer, copied_data);
		queue_transfer.clear();
	}

	bool SSBO_Buffer::is_storage_buffer_resizing(uint32_t frame_index) {
		return frame_resize_storage_buffer_info[frame_index].is_resizing;
	}

	void SSBO_Buffer::destroy() {
		inner_buffer.destroy();
		for (int i = 0; i < storage_buffers.size(); i++) {
			storage_buffers[i].destroy();
		}
		/**
		 * @Note: if device shutdown we need to recheck
		 * our queue buffer remove in flight again to
		 * remove all buffer!
		 */
		for (const auto& remove_buffer : queue_remove_inner_buffer) {
			remove_buffer.remove_buffer.destroy();
		}
		for (const auto& resize_info : frame_resize_storage_buffer_info) {
			if (resize_info.is_resizing) {
				resize_info.old_buffer.destroy();
			}
		}
	}

} // namespace Vulkan