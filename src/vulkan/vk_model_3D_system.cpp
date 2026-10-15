#include <math_custom.h>
#include <parser/gltf_parser.h>
#include <vulkan/vk_model_3D_system.h>

namespace Vulkan {

	void Model_3D_System::init(Texture_System* texture_system, Static_Buffer* static_buffer, SSBO_Buffer* ssbo) {
		this->texture_system = texture_system;
		this->static_buffer = static_buffer;
		this->ssbo = ssbo;
	}

	uint32_t Model_3D_System::load_model(std::string path) {
		if (files_to_ids.find(path) != files_to_ids.end()) {
			return files_to_ids[path];
		}
		uint32_t model_id = 0;
		if (available_ids.size() > 0) {
			model_id = available_ids.top();
			available_ids.pop();
		} else {
			model_id = ++id_counter;
		}
		Parser::Gltf_Model model = Parser::parse_gltf_model(path);
		Model_Information model_info{};
		model_info.global_meshes_transformes = std::move(model.make_meshes_global_transform());
		const auto& global_meshes_transformes = model_info.global_meshes_transformes;
		for (const auto& [scene_index, meshes_transforms] : global_meshes_transformes) {
			for (const auto& [mesh_index, transforms] : meshes_transforms) {
				auto& mesh = model_info.meshes[mesh_index];
				glm::vec3 origin = mesh.get_mesh_origin();
				std::vector<glm::mat4> transforms_data{};
				for (const glm::mat4& transform : transforms) {
					transforms_data.push_back(transform * Math::make_translation(-origin));
				}
				mesh.local_transform_storage_ids_by_scene.push_back(ssbo->upload_data(
					transforms_data.data(), transforms_data.size() * sizeof(glm::mat4), sizeof(glm::mat4)
				));
			}
		}
		for (int mesh_index = 0; mesh_index < model.meshes.size(); mesh_index++) {
			Parser::Gltf_Mesh& mesh = model.meshes[mesh_index];
			std::vector<Primitive_Buffer_Range> primitives;
			for (int primitive_index = 0; primitive_index < mesh.primitives.size(); primitive_index++) {
				Parser::Gltf_Primitive& primitive = mesh.primitives[primitive_index];
				uint32_t vertex_id = static_buffer->upload_data(
					primitive.vertices.data(), sizeof(Geometry::Vertex_3D) * primitive.vertices.size(),
					sizeof(Geometry::Vertex_3D)
				);
				uint32_t indices_id = static_buffer->upload_data(
					primitive.indices.data(), sizeof(uint32_t) * primitive.indices.size(), sizeof(uint32_t)
				);
				std::string texture = model.get_primitive_texture_path(mesh_index, primitive_index);
				uint32_t texture_id = texture_system->get_default_texture_id();
				if (texture != "") {
					texture_id = texture_system->load_texture(texture);
				}
				primitives.push_back({vertex_id, indices_id, texture_id});
			}
			model_info.meshes.push_back({std::move(primitives), mesh.min_bounding_box, mesh.max_bounding_box});
		}
		model_info.draw_scene_index = model.default_scene_index;
		for (const auto& mesh : model_info.meshes) {
			for (const auto& primitive : mesh.primitives) {
				model_info.flat_primitives.push_back(primitive);
			}
		}
		models[model_id] = model_info;
		ids_to_files[model_id] = path;
		files_to_ids[path] = model_id;
		return model_id;
	}

	const Model_Information& Model_3D_System::view_model(uint32_t model_id) {
		if (models.find(model_id) == models.end()) {
			throw std::runtime_error(
				("Model_Information::view_model Fail to get model " + std::to_string(model_id)).data()
			);
		}
		return models[model_id];
	}

	void Model_3D_System::remove_model(uint32_t model_id) {
		if (models.find(model_id) == models.end()) {
			return;
		}
		Model_Information& model_info = models[model_id];
		for (auto& mesh : model_info.meshes) {
			for (auto& primitive : mesh.primitives) {
				static_buffer->remove_data(primitive.vertex_id);
				static_buffer->remove_data(primitive.indices_id);
			}
		}
		models.erase(model_id);
		files_to_ids.erase(ids_to_files[model_id]);
		ids_to_files.erase(model_id);
		available_ids.push(model_id);
	}
} // namespace Vulkan