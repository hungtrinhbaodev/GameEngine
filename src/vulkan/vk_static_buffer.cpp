#include <stdexcept>
#include <vulkan/vk_static_buffer.h>

namespace Vulkan {

	void Static_Buffer::init(
		uint32_t initialize_size, uint32_t initialize_staging_size, VkBufferUsageFlags usage_flags,
		VkPhysicalDevice physical_device, VkDevice device
	) {
		staging_buffer.init(initialize_staging_size, physical_device, device);
		inner_buffer.make_buffer(
			initialize_size, usage_flags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physical_device, device
		);
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
		if (id_to_ssbo_id.has(id)) {
			uint32_t ssbo_id = id_to_ssbo_id.get(id);
			ssbo_id_to_id.erase(ssbo_id);
			id_to_ssbo_id.erase(id);
		}
		Static_Buffer_Range range = id_to_ranges.get(id);
		available_ranges.push(range);
		id_to_ranges.erase(id);
		range_id_generator.release_id(id);
		return true;
	}

	void Static_Buffer::flush_data() {
		if (inner_buffer.size < size_required) {
			uint32_t size = (uint32_t)size_required * 1.5f;
			inner_buffer.resize(size);
		}
		for (const uint32_t ssbo_id : ssbo_id_to_id.keys()) {
			SSBO_Buffer_Range ssbo_range = staging_buffer.view_slot(ssbo_id);
			uint32_t range_id = ssbo_id_to_id.get(ssbo_id);
			Static_Buffer_Range static_range = id_to_ranges.get(range_id);
			staging_buffer.transfer_data_to(ssbo_id, inner_buffer.buffer, static_range.inner_offset());
		}
		staging_buffer.flush_transfer_data();
		id_to_ssbo_id.clear();
		ssbo_id_to_id.clear();
		staging_buffer.clear();
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