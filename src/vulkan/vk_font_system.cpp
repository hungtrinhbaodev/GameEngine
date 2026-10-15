#include <utils.h>
#include <vulkan/vk_font_system.h>

namespace Vulkan {

	void Font_System::init(
		SSBO_Buffer* ssbo, VkDevice device, std::vector<VkDescriptorPool> descriptor_pools,
		std::array<VkDescriptorPool, Const::MAX_FRAMES_IN_FLIGHT> descriptor_pools_2, VkPhysicalDevice physical_device
	) {
		this->ssbo = ssbo;
		texture_system.init(
			Const::FONT_TEXTURE_BUCKET_SIZE, Const::NUMBER_LAYER_FONT_TEXTURE_PER_BUCKETS, device, descriptor_pools,
			descriptor_pools_2, physical_device, VK_FORMAT_R8_UNORM
		);
	}

	uint32_t Font_System::load_font(std::string font_path) {
		if (font_path == "") {
			font_path = Const::PATH_DEFAULT_FONT;
		}
		if (file_to_ids.find(font_path) != file_to_ids.end()) {
			return file_to_ids[font_path];
		}
		uint32_t font_id = font_id_generator.gen_id();
		Parser::Font font = Parser::parse_font(::Utils::get_root_path() + font_path);
		uint32_t texture_id = texture_system.load_texture(
			font_path, font.atlas.data(), font.size_atlas, font.size_atlas, 1, Const::ENABLED_TEXTURE_BUCKETS
		);
		std::vector<glm::vec4> rects;
		for (const auto& rect : font.rects) {
			rects.push_back({rect.tex_coord_x, rect.tex_coord_y, rect.tex_coord_width, rect.tex_coord_height});
		}
		uint32_t rect_storage_id = ssbo->upload_data(rects.data(), sizeof(glm::vec4) * rects.size(), sizeof(glm::vec4));
		const Buffer_Range& storage_range = ssbo->view_slot(rect_storage_id);
		uint32_t rect_storage_offset = storage_range.offset_as<glm::vec4>();
		file_to_ids[font_path] = font_id;
		return fonts.insert(font_id, {std::move(font), texture_id, rect_storage_id, rect_storage_offset});
	}

	const Font& Font_System::view_font(uint32_t font_id) {
		return fonts.get(font_id);
	}

	void Font_System::destroy() {
		texture_system.destroy();
	}

} // namespace Vulkan