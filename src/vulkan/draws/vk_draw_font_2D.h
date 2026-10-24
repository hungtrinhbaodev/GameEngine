#pragma once
#include <vulkan/vk_draw.h>

namespace Vulkan {

	namespace Draw_Font_2D {

		struct Text_Helper {
			std::vector<std::string> lines{};
			std::vector<glm::vec2> start_line_positions{};
			float width = 0;
			float height = 0;
		};

		bool is_material_equal(uint32_t a, uint32_t b);

		void init();

		size_t get_instance_size();

		void draw(
			VkCommandBuffer command_buffer, uint32_t material_draw_id, uint32_t number_instance,
			uint32_t first_instance_offset, int frame_index
		);

		void init_font(
			Draw_Information& draw_info, std::vector<Draw_Instance_Information>& draw_instances, Label_2D& attributes
		);

		void update_font(
			Draw_Information& draw_info, std::vector<Draw_Instance_Information>& draw_instances, Label_2D& attributes
		);

		void remove_font(uint32_t sub_unique_id);

		void destroy();

		void break_lines(std::string text, std::vector<std::string>& lines);

		glm::vec2 get_line_size(const std::string& line, const Font& font);

		std::vector<glm::vec2> get_char_position_at(std::string line, glm::vec2 start_line_position, const Font& font);

		void make_lines_render_info(
			const Font& font, int align, int font_size, std::vector<glm::vec2>& start_line_positions, float& width,
			float& height, const std::vector<std::string>& lines
		);

		Text_Helper gen_text_info_from(std::string font_path, std::string text, int align, int font_size);

	} // namespace Draw_Font_2D

} // namespace Vulkan