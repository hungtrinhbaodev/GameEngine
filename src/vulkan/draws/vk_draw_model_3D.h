#pragma once
#include <vulkan/draws/vk_draw_attributes.h>
#include <vulkan/vk_common.h>

namespace Vulkan {

	namespace Draw_Model_3D {

		struct Model_Animation_Helper {
			std::string animation_name = "";
			float duration = 0.f;
		};

		struct Model_Helper {
			std::vector<std::string> scene_names;
			std::vector<Model_Animation_Helper> animations;
			bool has_animation(const std::string& animation_name) const;
			const Model_Animation_Helper& get_animation_heler(const std::string& animation_name) const;
		};

		bool is_material_equal(uint32_t a, uint32_t b);

		void init();

		size_t get_instance_size();

		void setup_draw(uint32_t frame_index, float dt);

		void record_upload_uniform_buffer(VkCommandBuffer command_buffer, uint32_t frame_index);

		uint32_t make_instance_material_id(uint32_t model_id, uint32_t primitive_index);

		void resovle_instance_material_id(uint32_t instance_material_id, uint32_t& model_id, uint32_t& primitive_index);

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

		const Model_Helper& get_model_helper(const std::string& model_path);

		void make_model_helper(const std::string& model_path);

	} // namespace Draw_Model_3D

} // namespace Vulkan