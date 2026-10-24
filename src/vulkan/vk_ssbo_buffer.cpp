#include <profiler.h>
#include <stdexcept>
#include <vulkan/vk_ssbo_buffer.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

	void SSBO_Buffer::init(uint32_t initialize_size, VkPhysicalDevice physical_device, VkDevice device) {
		this->device = device;
		for (int i = 0; i < storage_buffers.size(); i++) {
			storage_buffers[i].make_buffer(
				initialize_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				physical_device, device
			);
		}
		for (int i = 0; i < staging_buffers.size(); i++) {
			staging_buffers[i].make_buffer(
				initialize_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, physical_device, device,
				true
			);
		}
	}

	void SSBO_Buffer::setup_frame(uint32_t current_frame) {
		this->current_frame = current_frame;
	}

	uint32_t SSBO_Buffer::upload_data(const void* data, uint32_t size, uint32_t element_stride) {
		element_stride = std::max(element_stride, (uint32_t)1);
		uint32_t range_id = range_id_generator.gen_id();
		Buffer_Range using_range =
			Utils::find_suitable_buffer_range(available_ranges, current_offset, size, element_stride);
		uint32_t size_required = using_range.offset + using_range.total_size();
		/**
		 * Use ram bytes data storage here!
		 */
		if (bytes_data.size() < size_required) {
			uint32_t new_size = (uint32_t)(size_required * 1.5f);
			bytes_data.resize(new_size, 0);
		}
		memcpy(bytes_data.data() + using_range.inner_offset(), data, (size_t)using_range.size);
		for (auto& dirty_ids : frame_dirty_ids) {
			dirty_ids.insert(range_id, true);
		}
		return ranges_by_id.insert(range_id, using_range);
	}

	void SSBO_Buffer::update_data(uint32_t id, const void* data, uint32_t offset, uint32_t size) {
		Buffer_Range range = view_slot(id);
		if (size <= 0) {
			size = range.inner_size();
		}
		if (offset <= 0) {
			offset = range.inner_offset();
		}
		memcpy(bytes_data.data() + offset, data, (size_t)size);
		for (auto& dirty_ids : frame_dirty_ids) {
			dirty_ids.insert(id, true);
		}
	}

	void SSBO_Buffer::remove_data(uint32_t id) {
		if (!ranges_by_id.has(id))
			return;
		Buffer_Range range = ranges_by_id.get(id);
		available_ranges.push(range);
		range_id_generator.release_id(id);
		ranges_by_id.erase(id);
		for (int i = 0; i < frame_dirty_ids.size(); i++) {
			frame_dirty_ids[i].erase(id);
		}
	}

	const Buffer_Range& SSBO_Buffer::view_slot(uint32_t id) {
		return ranges_by_id.get(id);
	}

	void SSBO_Buffer::flush_data_to_staging_buffer(uint32_t current_frame) {
		Profiler::start_scope(Const::SCOPE_SSBO_COPY_DATA_TO_STAGINGS);
		Buffer& staging_buffer = staging_buffers[current_frame];
		if (staging_buffer.size < bytes_data.size()) {
			staging_buffer.resize(bytes_data.size(), true);
		}
		copy_ranges.clear();
		Sparse_Set<bool>& dirty_ids = frame_dirty_ids[current_frame];
		uint8_t* staging_mapped = (uint8_t*)staging_buffer.mapped;
		uint8_t* bytes_mapped = (uint8_t*)bytes_data.data();
		for (const uint32_t& id : dirty_ids.keys()) {
			const Buffer_Range& range = view_slot(id);
			uint8_t* src = bytes_mapped + range.inner_offset();
			uint8_t* dst = staging_mapped + range.inner_offset();
			memcpy(dst, src, range.inner_size());
			copy_ranges.push_back({range.inner_offset(), range.inner_offset(), range.inner_size()});
		}
		dirty_ids.clear();
		Profiler::end_scope(Const::SCOPE_SSBO_COPY_DATA_TO_STAGINGS);
	}

	void SSBO_Buffer::flush_data_to_storage_buffer(VkCommandBuffer command_buffer, uint32_t current_frame) {
		Profiler::start_scope(Const::SCOPE_SSBO_COPY_DATA_TO_STORES);
		Resize_Buffer_Synchronize_Information& resize_info = frame_resize_storage_buffer_info[current_frame];
		if (resize_info.is_resizing) {
			resize_info.old_buffer.destroy();
			resize_info.is_resizing = false;
		}
		Buffer& storage_buffer = storage_buffers[current_frame];
		if (storage_buffer.size < bytes_data.size()) {
			resize_info.old_buffer = storage_buffer.record_resize(command_buffer, bytes_data.size());
			resize_info.is_resizing = true;
		}
		if (copy_ranges.size() <= 0) {
			return;
		}
		Buffer& staging_buffer = staging_buffers[current_frame];
		Utils::record_buffer_barrier(
			command_buffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, storage_buffer.buffer, 0, storage_buffer.size
		);
		{
			vkCmdCopyBuffer(
				command_buffer, staging_buffer.buffer, storage_buffer.buffer, copy_ranges.size(), copy_ranges.data()
			);
		}
		Utils::record_buffer_barrier(
			command_buffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, storage_buffer.buffer, 0, storage_buffer.size
		);
		Profiler::end_scope(Const::SCOPE_SSBO_COPY_DATA_TO_STORES);
	}

	bool SSBO_Buffer::is_storage_buffer_resizing(uint32_t frame_index) {
		return frame_resize_storage_buffer_info[frame_index].is_resizing;
	}

	void SSBO_Buffer::destroy() {
		for (int i = 0; i < storage_buffers.size(); i++) {
			storage_buffers[i].destroy();
		}
		for (int i = 0; i < staging_buffers.size(); i++) {
			staging_buffers[i].destroy();
		}
		/**
		 * @Note: if device shutdown we need to recheck
		 * our queue buffer remove in flight again to
		 * remove all buffer!
		 */
		for (const auto& resize_info : frame_resize_storage_buffer_info) {
			if (resize_info.is_resizing) {
				resize_info.old_buffer.destroy();
			}
		}
	}

} // namespace Vulkan