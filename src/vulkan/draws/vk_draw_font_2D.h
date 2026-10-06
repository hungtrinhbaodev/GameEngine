#pragma once
#include <vulkan/draws/draw_structs.h>
#include <vulkan/vk_draw_2D.h>

namespace Vulkan {

	namespace Draw_2D {

		namespace Font_2D {

			bool is_material_equal(uint32_t a, uint32_t b);

			void init();

			size_t get_instance_size();

			void draw(
				VkCommandBuffer command_buffer, uint32_t material_draw_id, uint32_t number_instance,
				uint32_t first_instance_offset, int frame_index
			);

			Draw_2D_Information make_font_2D(const Font_2D_Attributes& texture_attributes);

			void destroy();

		} // namespace Font_2D

	} // namespace Draw_2D

} // namespace Vulkan