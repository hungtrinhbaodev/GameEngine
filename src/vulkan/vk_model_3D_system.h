#pragma once
#include <map>
#include <stack>
#include <string>
#include <vulkan/vk_static_buffer.h>
#include <vulkan/vk_texture_system.h>

namespace Vulkan {

	struct Primitive_Buffer_Range {
		uint32_t vertex_id = 0;
		uint32_t indices_id = 0;
		uint32_t texture_id = 0;
	};

	struct Model_Mesh_Information {
		std::vector<Primitive_Buffer_Range> primitives;
		glm::vec3 min_bounding_box{0.f};
		glm::vec3 max_bounding_box{0.f};
		glm::vec3 get_mesh_origin() const;
	};

	struct Model_Information {
		std::vector<Model_Mesh_Information> meshes;
		std::map<uint32_t, std::map<uint32_t, std::vector<glm::mat4>>> global_meshes_transform;
		uint32_t draw_scene_index = -1;
	};

	struct Model_3D_System {

		Static_Buffer* global_vertex_buffer = nullptr;

		Static_Buffer* global_indices_buffer = nullptr;

		Static_Buffer_2* static_buffer = nullptr;

		Texture_System* texture_system = nullptr;

		std::stack<uint32_t> available_ids;

		uint32_t id_counter = 0;

		std::map<uint32_t, std::string> ids_to_files;

		std::map<std::string, uint32_t> files_to_ids;

		std::map<uint32_t, Model_Information> models;

		void init(
			Static_Buffer* global_vertex_buffer, Static_Buffer* global_indices_buffer, Texture_System* texture_system,
			Static_Buffer_2* static_buffer
		);

		uint32_t load_model(std::string path);

		const Model_Information& view_model(uint32_t model_id);

		void remove_model(uint32_t model_id);
	};

} // namespace Vulkan