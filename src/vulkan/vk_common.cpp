#include <stdexcept>
#include <vulkan/vk_common.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {
	void Resource_Synchronize_Information::release_frame(uint32_t frame_index) {
		if (frame_index < 0 || frame_index > blocked.size())
			return;
		blocked[frame_index] = false;
	}

	bool Resource_Synchronize_Information::can_remove() {
		for (bool block : blocked) {
			if (block) {
				return false;
			}
		}
		return true;
	}

	uint32_t Buffer_Range::total_size() const {
		return align + size;
	}

	bool Buffer_Range::is_suitable_with(uint32_t size, uint32_t element_stride) const {
		element_stride = std::max(uint32_t(1), element_stride);
		uint32_t align = Utils::calculate_range_remain_size(offset, element_stride);
		return total_size() >= size + align;
	}

	Buffer_Range Buffer_Range::fit_with(uint32_t size, uint32_t element_stride) {
		if (!is_suitable_with(size, element_stride)) {
			throw std::runtime_error("Fail to fix static data in to current range!");
		}
		uint32_t new_align = 0;
		uint32_t new_size = size;
		element_stride = std::max(uint32_t(1), element_stride);
		new_align = Utils::calculate_range_remain_size(offset, element_stride);
		uint32_t remain_offset = offset + (new_align + new_size);
		uint32_t remain_size = total_size() - (new_align + new_size);
		this->align = new_align;
		this->size = new_size;
		return {remain_offset, 0, remain_size};
	}

	uint32_t Buffer_Range::inner_offset() {
		return offset + align;
	}

	uint32_t Buffer_Range::inner_size() {
		return size;
	}

	bool Buffer_Range_Compare::operator()(const Buffer_Range& a, const Buffer_Range& b) const {
		return a.total_size() > b.total_size();
	}
} // namespace Vulkan