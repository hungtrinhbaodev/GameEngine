#pragma once
#include <id_generator.h>
#include <map>
#include <sparse_set.h>
#include <stack>
#include <string>
#include <vulkan/vk_common.h>
#include <vulkan/vk_static_buffer.h>
#include <vulkan/vk_texture_system.h>

namespace Vulkan {

	struct Model_3D_System {

		SSBO_Buffer* ssbo = nullptr;

		Static_Buffer* static_buffer = nullptr;

		Texture_System* texture_system = nullptr;

		std::unordered_map<uint32_t, std::string> ids_to_files;

		std::unordered_map<std::string, uint32_t> files_to_ids;

		std::unordered_map<uint32_t, std::string> ids_to_animation_files;

		std::unordered_map<std::string, uint32_t> animation_files_to_ids;

		Id_Generator model_id_generator{};

		Sparse_Set<Model> models{};

		Id_Generator animation_id_generator{};

		Sparse_Set<Model_Animation_Component> animations{};

		void init(Texture_System* texture_system, Static_Buffer* static_buffer, SSBO_Buffer* ssbo);

		uint32_t load_model(const std::string& path);

		uint32_t load_animation(const std::string& path);

		void attach_component(uint32_t animations_id, uint32_t to_model_id);

		const Model& view_model(uint32_t model_id);

		const Model_Animation_Component& view_animation(uint32_t animations_id);

		void remove_model(uint32_t model_id);
	};

} // namespace Vulkan