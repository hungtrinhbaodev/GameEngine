#include <utils.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_font_system.h>

namespace Vulkan {

	Geometry::Texture_Rect_2D Font::get_char_rect(char value) const {
		Parser::Char_Rect rect = rect_map.at('*');
		if (rect_map.find(value) != rect_map.end()) {
			rect = rect_map.at(value);
		}
		return {rect.tex_coord_x, rect.tex_coord_y, rect.tex_coord_width, rect.tex_coord_height};
	}

	glm::vec2 Font::get_char_bounding_box(char value) const {
		Parser::Char_Rect rect = rect_map.at('*');
		if (rect_map.find(value) != rect_map.end()) {
			rect = rect_map.at(value);
		}
		return {rect.bounding_width, line_height};
	}

	glm::vec2 Font::get_char_local_offset(char value) const {
		Parser::Char_Rect rect = rect_map.at('*');
		if (rect_map.find(value) != rect_map.end()) {
			rect = rect_map.at(value);
		}
		return {rect.offset_x, rect.offset_y};
	}

	float Font::get_char_scale(int font_size) const {
		font_size = std::max(std::min(font_size, max_font_size), min_font_size);
		return (float)font_size / max_font_size;
	}

	void Font_System::init(VkDevice device, std::vector<VkDescriptorPool> descriptor_pools) {
		texture_system.init(
			Const::FONT_TEXTURE_BUCKET_SIZE, Const::NUMBER_LAYER_FONT_TEXTURE_PER_BUCKETS, device, descriptor_pools,
			VK_FORMAT_R8_UNORM
		);
	}

	uint32_t Font_System::load_font(std::string font_path) {
		if (font_path == "") {
			font_path = Const::PATH_DEFAULT_FONT;
		}
		if (file_to_ids.find(font_path) != file_to_ids.end()) {
			return file_to_ids[font_path];
		}
		Parser::Font font = Parser::parse_font(::Utils::get_root_path() + font_path);
		uint32_t texture_id = texture_system.load_texture(
			font_path, font.atlas.data(), font.size_atlas, font.size_atlas, 1, Const::ENABLED_TEXTURE_BUCKETS
		);
		uint32_t font_id = fonts.insert({std::move(font), texture_id});
		file_to_ids[font_path] = font_id;
		return font_id;
	}

	const Font& Font_System::view_font(uint32_t font_id) {
		return fonts.get(font_id);
	}

	void Font_System::destroy() {
		texture_system.destroy();
	}

} // namespace Vulkan