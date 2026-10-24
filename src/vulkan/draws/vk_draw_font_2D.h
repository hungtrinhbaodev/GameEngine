#pragma once
#include <vulkan/draws/draw_structs.h>
#include <vulkan/vk_draw.h>

namespace Vulkan {

	namespace Font_2D {

		bool is_material_equal(uint32_t a, uint32_t b);

		void init();

		size_t get_instance_size();

		void draw(
			VkCommandBuffer command_buffer, uint32_t material_draw_id, uint32_t number_instance,
			uint32_t first_instance_offset, int frame_index
		);

		std::vector<Draw_Instance_Information> make_font_2D(
			Draw_Information& draw_info, const Font_2D_Attributes& text_attributes
		);

		std::vector<Draw_Instance_Information> update_font_2D(
			Draw_Information& draw_info, const Font_2D_Attributes& text_attributes
		);

		void init_font(
			Draw_Information& draw_info, std::vector<Draw_Instance_Information>& draw_instances,
			const Label_2D& attributes
		);

		void update_font(
			Draw_Information& draw_info, std::vector<Draw_Instance_Information>& draw_instances,
			const Label_2D& attributes
		);

		void remove_font(uint32_t sub_unique_id);

		void destroy();

	} // namespace Font_2D

} // namespace Vulkan