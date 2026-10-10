#pragma once
#include <vulkan/draws/draw_structs.h>
#include <vulkan/vk_draw.h>
#include <vulkan/vulkan.h>

namespace Vulkan {

	namespace Triangle {

		bool is_material_equal(uint32_t a, uint32_t b);

		void init();

		size_t get_instance_size();

		Draw_Information make_triangle(const Triangle_Attribultes& attributes);

		void update_triangle(uint32_t instance_id, const Triangle_Attribultes& attributes);

		void draw(
			VkCommandBuffer command_buffer, uint32_t material_draw_id, uint32_t number_instance,
			uint32_t first_instance_offset
		);

		void destroy();

	} // namespace Triangle

} // namespace Vulkan