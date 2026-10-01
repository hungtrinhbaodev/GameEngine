#include <stdexcept>
#include <vulkan/vk_static_buffer.h>

namespace Vulkan {

	void Static_Buffer::init(
		Ring_Buffer* global_staging_buffer, uint32_t initialize_size, VkBufferUsageFlags usage_flags
	) {

		available_size = initialize_size;
		staging_buffer = global_staging_buffer;

		inner_buffer.make_buffer(available_size, usage_flags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		id_counter = 0;
		current_offset = 0;
	}

	uint32_t Static_Buffer::upload_data(uint32_t size, void* data) {

		uint32_t id = 0;
		if (available_ids.size() > 0) {
			id = available_ids.top();
			available_ids.pop();
		} else {
			id = id_counter++;
		}

		Static_Buffer_Range using_range{0, 0};
		if (available_ranges.size() > 0 && available_ranges.top().size >= size) {
			Static_Buffer_Range optimal_range = available_ranges.top();
			available_ranges.pop();
			if (optimal_range.size > size) {
				Static_Buffer_Range remain_range{optimal_range.offset + size, optimal_range.size - size};
				available_ranges.push(remain_range);
				optimal_range.size = size;
			}
			using_range = optimal_range;

		} else {
			using_range = {current_offset, size};
			if (current_offset + size > available_size) {
				available_size = (uint32_t)((current_offset + size) * 1.5f);
				VkBuffer current_buffer = inner_buffer.buffer;
				inner_buffer.resize(available_size);
				VkBuffer updated_buffer = inner_buffer.buffer;
				staging_buffer->update_dst_buffer_transfer(current_buffer, updated_buffer);
			}
			current_offset += size;
		}

		ranges_by_id[id] = using_range;
		staging_buffer->upload_data(inner_buffer.buffer, using_range.offset, size, data);

		if (id < 0) {
			throw std::runtime_error("Vulkan fail to upload static data: fail to get id!");
		}

		return static_cast<uint32_t>(id);
	}

	Static_Buffer_Range Static_Buffer::view_slot_info(uint32_t id) {

		if (ranges_by_id.find(id) == ranges_by_id.end()) {
			return {};
		}
		return ranges_by_id[id];
	}

	bool Static_Buffer::remove_data(uint32_t id) {

		if (ranges_by_id.find(id) == ranges_by_id.end()) {
			return false;
		}

		available_ranges.push(ranges_by_id[id]);
		ranges_by_id.erase(id);
		available_ids.push(id);

		return true;
	}

	void Static_Buffer::destroy() {
		inner_buffer.destroy();
	}
} // namespace Vulkan