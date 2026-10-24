#pragma once
#include <vulkan/draws/vk_draw_attributes.h>
#include <vulkan/vk_common.h>
#include <vulkan/vulkan.h>

namespace Vulkan {

	namespace Draw_Texture_2D {

		bool is_material_equal(uint32_t a, uint32_t b);

		void init();

		size_t get_instance_size();

		void draw(
			VkCommandBuffer command_buffer, uint32_t material_draw_id, uint32_t number_instance,
			uint32_t first_instance_offset, uint32_t frame_index
		);

		void init_texture(
			Draw_Information& draw_info, std::vector<Draw_Instance_Information>& draw_instances,
			const Texture_2D& attributes
		);

		void update_texture(Draw_Information& draw_info, const Texture_2D& attributes);

		void remove_texture(uint32_t sub_unique_id);

		void destroy();

	} // namespace Draw_Texture_2D

} // namespace Vulkan