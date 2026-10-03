#include <stdexcept>
#include <vulkan/vk_ssbo_buffer.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

	void SSBO_Buffer::init(uint32_t initialize_size) {
		inner_buffer.make_buffer(
			initialize_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		);
	}

	uint32_t SSBO_Buffer::upload_data(void* data, uint32_t size) {
		uint32_t id = id_generator.gen_id();
		SSBO_Buffer_Range using_range{0, 0};
		if (available_ranges.size() > 0 && available_ranges.top().size >= size) {
			SSBO_Buffer_Range optimal_range = available_ranges.top();
			available_ranges.pop();
			if (optimal_range.size > size) {
				SSBO_Buffer_Range remain_range{optimal_range.offset + size, optimal_range.size - size};
				available_ranges.push(remain_range);
				optimal_range.size = size;
			}
			using_range = optimal_range;
		} else {
			using_range = {current_offset, size};
			if (current_offset + size > inner_buffer.size) {
				uint32_t buffer_size = (uint32_t)((current_offset + size) * 1.5f);
				inner_buffer.resize(buffer_size);
			}
			current_offset += size;
		}
		inner_buffer.copy_data(using_range.size, data, using_range.offset);
		ranges_by_id[id] = using_range;
		return id;
	}

	void SSBO_Buffer::update_data(uint32_t id, void* data, uint32_t offset, uint32_t size) {
		SSBO_Buffer_Range range = view_slot(id);
		if (size <= 0) {
			size = range.size;
		}
		if (offset <= 0) {
			offset = range.offset;
		}
		inner_buffer.copy_data(size, data, offset);
	}

	void SSBO_Buffer::remove_data(uint32_t id) {
		if (ranges_by_id.find(id) == ranges_by_id.end()) {
			return;
		}
		SSBO_Buffer_Range range = ranges_by_id[id];
		available_ranges.push(range);
		id_generator.release_id(id);
		ranges_by_id.erase(id);
	}

	SSBO_Buffer_Range SSBO_Buffer::view_slot(uint32_t id) {
		if (ranges_by_id.find(id) == ranges_by_id.end()) {
			throw std::runtime_error(
				"Fail to view SSBO buffer range with id: " + std::to_string(id) + " please check!"
			);
		}
		return ranges_by_id[id];
	}

	void SSBO_Buffer::transfer_data_to(uint32_t id, VkBuffer dst_buffer, uint32_t dst_offset) {
		SSBO_Buffer_Range range = view_slot(id);
		Transfer_Data_Information transfer_data{dst_offset, range.offset, range.size, dst_buffer};
		queue_transfer.push_back(transfer_data);
	}

	void SSBO_Buffer::update_dst_buffer_transfer(VkBuffer from, VkBuffer to) {
		for (Transfer_Data_Information& transfer : this->queue_transfer) {
			if (transfer.dst_buffer == from) {
				transfer.dst_buffer = to;
			}
		}
	}

	void SSBO_Buffer::flush_transfer_data() {
		std::map<VkBuffer, std::vector<VkBufferCopy>> copied_data;
		for (const Transfer_Data_Information& transfer : queue_transfer) {
			VkBuffer dst_buffer = transfer.dst_buffer;
			if (copied_data.find(dst_buffer) == copied_data.end()) {
				copied_data[dst_buffer] = {};
			}
			VkBufferCopy region{
				static_cast<VkDeviceSize>(transfer.src_offset), static_cast<VkDeviceSize>(transfer.dst_offset),
				static_cast<VkDeviceSize>(transfer.size_transfer)
			};
			copied_data[dst_buffer].push_back(region);
		}
		if (copied_data.size() <= 0) {
			queue_transfer.clear();
			return;
		}
		Utils::copy_data_to_multi_buffer(inner_buffer.buffer, copied_data);
		queue_transfer.clear();
	}

	void SSBO_Buffer::destroy() {
		inner_buffer.destroy();
	}

} // namespace Vulkan