#pragma once
#include <vulkan/draws/draw_structs.h>
#include <vulkan/vk_draw.h>

namespace Vulkan {

	namespace Model_3D {

		bool is_material_equal(uint32_t a, uint32_t b);

		void init();

		size_t get_instance_size();

		Draw_Information make_model_3D(const Model_3D_Attributes& attributes);

		void setup_draw(uint32_t frame_index);

		void draw(
			VkCommandBuffer command_buffer, uint32_t material_draw_id, uint32_t number_instance,
			uint32_t first_instance_offset, uint32_t frame_index
		);

		void destroy();

	} // namespace Model_3D

} // namespace Vulkan