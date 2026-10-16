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

	glm::vec2 Texture_View::tex_scale() const {
		return {width / image.width, height / image.height};
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
		this->stride = element_stride;
		return {remain_offset, 0, remain_size, 1};
	}

	uint32_t Buffer_Range::inner_offset() const {
		return offset + align;
	}

	uint32_t Buffer_Range::inner_size() const {
		return size;
	}

	bool Buffer_Range_Compare::operator()(const Buffer_Range& a, const Buffer_Range& b) const {
		return a.total_size() > b.total_size();
	}

	char Font::to_char_index(char value) const {
		uint32_t char_index = (uint32_t)(value - start_char);
		if (char_index >= rects.size()) {
			char_index = (uint32_t)('*' - start_char);
		}
		return char_index;
	}

	glm::vec2 Font::get_char_bounding_box(char value) const {
		uint32_t char_index = to_char_index(value);
		const auto& rect = rects[char_index];
		return {rect.bounding_width, line_height};
	}

	glm::vec2 Font::get_char_local_offset(char value) const {
		uint32_t char_index = to_char_index(value);
		const auto& rect = rects[char_index];
		return {rect.offset_x, rect.offset_y};
	}

	float Font::get_char_scale(int font_size) const {
		font_size = std::max(std::min(font_size, max_font_size), min_font_size);
		return (float)font_size / max_font_size;
	}

	uint32_t Font::get_char_rect_offset(char value) const {
		uint32_t char_index = to_char_index(value);
		return rect_storage_offset + char_index;
	}

	glm::vec3 Model_Mesh_Information::get_mesh_origin() const {
		return (max_bounding_box + min_bounding_box) * 0.5f;
	}

	const std::vector<uint32_t>& Model_Mesh_Information::get_mesh_transform_offsets(uint32_t scene_index) const {
		return mesh_transform_storage_offsets.at(scene_index);
	}

	uint32_t Model_Information::get_number_primitive() const {
		return flat_primitives.size();
	}

	uint32_t Model_Information::get_primitive_flat_index(
		const uint32_t& mesh_index, const uint32_t& primitive_index
	) const {
		uint32_t flat_index = 0;
		for (int i = 0; i < mesh_index; i++) {
			flat_index += meshes[i].primitives.size();
		}
		return flat_index + primitive_index;
	}
} // namespace Vulkan