#pragma once
#include <glm/glm.hpp>
#include <vulkan/vk_draw.h>
#include <vulkan/vulkan.h>

namespace Vulkan {

	namespace Rectangle {

		bool is_material_equal(uint32_t a, uint32_t b);

		void init();

		size_t get_instance_size();

		Draw_Information make_draw(const Rectangle_Attributes& rectangle_attributes);

		void update_draw(uint32_t instance_id, const Rectangle_Attributes& rectangle_attributes);

		void draw(
			VkCommandBuffer command_buffer, uint32_t material_draw_id, uint32_t number_instance,
			uint32_t first_instance_offset
		);

		void destroy();

	} // namespace Rectangle

} // namespace Vulkan