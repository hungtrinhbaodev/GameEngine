#pragma once
#include <vulkan/draws/draw_structs.h>
#include <vulkan/draws/vk_draw_attributes.h>
#include <vulkan/vk_common.h>

namespace Vulkan {

	namespace Draw_Model_3D {

		bool is_material_equal(uint32_t a, uint32_t b);

		void init();

		size_t get_instance_size();

		void setup_draw(uint32_t frame_index, float dt);

		void record_upload_uniform_buffer(VkCommandBuffer command_buffer, uint32_t frame_index);

		void draw(
			VkCommandBuffer command_buffer, uint32_t material_draw_id, uint32_t instance_material_id,
			uint32_t number_instance, uint32_t first_instance_offset, uint32_t frame_index
		);

		void init_model(
			Draw_Information& draw_info, std::vector<Draw_Instance_Information>& draw_instances,
			const Model_3D& attributes
		);

		void update_model(
			Draw_Information& draw_info, std::vector<Draw_Instance_Information>& draw_instances,
			const Model_3D& attributes
		);

		void remove_model(uint32_t unique_id);

		void destroy();

	} // namespace Draw_Model_3D

} // namespace Vulkan