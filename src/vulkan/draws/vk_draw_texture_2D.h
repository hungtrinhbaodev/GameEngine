#pragma once
#include <vulkan/vk_draw.h>
#include <vulkan/vulkan.h>

namespace Vulkan {

	namespace Texture_2D {

		bool is_material_equal(uint32_t a, uint32_t b);

		void init();

		size_t get_instance_size();

		Draw_Information make_texture_2D(const Texture_2D_Attributes& texture_attributes);

		void update_texture_2D(const Draw_Information draw_info, const Texture_2D_Attributes& texture_attributes);

		void draw(
			VkCommandBuffer command_buffer, uint32_t material_draw_id, uint32_t number_instance,
			uint32_t first_instance_offset, uint32_t frame_index
		);

		void destroy();

	} // namespace Texture_2D

} // namespace Vulkan