#pragma once
#include <glm/glm.hpp>
#include <vulkan/vk_common.h>
#include <vulkan/vulkan.h>

namespace Vulkan {

	namespace Rectangle {

		bool is_material_equal(uint32_t a, uint32_t b);

		void init();

		size_t get_instance_size();

		std::vector<Draw_Instance_Information> make_rectangle_2D(
			Draw_Information& draw_info, const Rectangle_Attributes& rectangle_attributes
		);

		void update_draw(uint32_t instance_id, const Rectangle_Attributes& rectangle_attributes);

		void draw(
			VkCommandBuffer command_buffer, uint32_t material_draw_id, uint32_t number_instance,
			uint32_t first_instance_offset, uint32_t frame_index
		);

		void destroy();

	} // namespace Rectangle

} // namespace Vulkan