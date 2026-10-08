#pragma once
#include <geometry_structs.h>
#include <id_generator.h>
#include <map>
#include <parser/font_parser.h>
#include <sparse_set.h>
#include <vulkan/vk_texture_system.h>

namespace Vulkan {

	struct Font : public Parser::Font {
		uint32_t texture_id = 0;
		Geometry::Texture_Rect_2D get_char_rect(char value) const;
		glm::vec2 get_char_bounding_box(char value) const;
		glm::vec2 get_char_local_offset(char value) const;
		float get_char_scale(int font_size) const;
	};

	struct Font_System {

		Id_Generator font_id_generator{};

		Texture_System texture_system{};

		Sparse_Set<Font> fonts{};

		std::unordered_map<std::string, uint32_t> file_to_ids{};

		void init(VkDevice device, std::vector<VkDescriptorPool> descriptor_pools, VkPhysicalDevice physical_device);

		uint32_t load_font(std::string font_path);

		const Font& view_font(uint32_t font_id);

		void destroy();
	};

} // namespace Vulkan