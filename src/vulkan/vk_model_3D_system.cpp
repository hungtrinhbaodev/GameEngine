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
		uint32_t model_id = id_generator.gen_id();
		Parser::Gltf_Model model = Parser::parse_gltf_model(path);
		Model model_info{};
		uint32_t primitive_count = 0;
		model_info.meshes.resize(model.meshes.size());
		for (int i = 0; i < model.meshes.size(); i++) {
			Parser::Gltf_Mesh& mesh = model.meshes[i];
			std::vector<Model_Primitive> primitives;
			std::vector<int> primitive_indices{};
			for (int j = 0; j < mesh.primitives.size(); j++) {
				Parser::Gltf_Primitive& primitive = mesh.primitives[j];
				uint32_t vertex_id = static_buffer->upload_data(
					primitive.vertices.data(), sizeof(Geometry::Vertex_3D) * primitive.vertices.size(),
					sizeof(Geometry::Vertex_3D)
				);
				uint32_t indices_id = static_buffer->upload_data(
					primitive.indices.data(), sizeof(uint32_t) * primitive.indices.size(), sizeof(uint32_t)
				);
				Parser::Gltf_Texture texture = model.get_primitive_texture(i, j);
				uint32_t texture_id = texture_system->get_default_texture_id();
				if (!texture.is_using_bin_texture) {
					if (texture.path != "") {
						texture_id = texture_system->load_texture(texture.path, Const::ENABLED_TEXTURE_BUCKETS);
					}
				} else {
					texture_id = texture_system->load_texture(
						texture.path, texture.data, texture.width, texture.height, 4, Const::ENABLED_TEXTURE_BUCKETS
					);
					delete (texture.data);
				}
				model_info.primitives.push_back({vertex_id, indices_id, texture_id});
				primitive_indices.push_back(primitive_count);
				primitive_count++;
			}
			model_info.meshes[i] = {std::move(primitive_indices), mesh.min_bounding_box, mesh.max_bounding_box};
		}
		model_info.default_scene_index = model.default_scene_index;
		model_info.default_skin_index = model.default_skin_index;
		model_info.animations.resize(model.animations.size());
		for (int i = 0; i < model_info.animations.size(); i++) {
			model_info.animations[i].parse_from(model.animations[i]);
		}
		model_info.skins.resize(model.skins.size());
		for (int i = 0; i < model_info.skins.size(); i++) {
			model_info.skins[i].parse_from(model.skins[i]);
		}
		model_info.nodes.resize(model.nodes.size());
		for (int i = 0; i < model_info.nodes.size(); i++) {
			model_info.nodes[i].parse_from(model.nodes[i]);
		}
		model_info.scenes.resize(model.scenes.size());
		std::vector<glm::mat4> transforms(model.nodes.size(), {1.f});
		for (int i = 0; i < model_info.scenes.size(); i++) {
			model_info.scenes[i].parse_from(model.scenes[i]);
			model_info.scene_index_to_draw_meshes_info.insert(i, {});
			model_info.dfs_write(i, [&](Model& model_info, int scene_index, int parent_index, int children_index) {
				if (parent_index == -1) {
					transforms[children_index] = model_info.nodes[children_index].get_transform();
				} else {
					transforms[children_index] =
						transforms[parent_index] * model_info.nodes[children_index].get_transform();
				}
				const auto& node = model_info.nodes[children_index];
				if (node.mesh_index >= 0) {
					auto& draws_info = model_info.scene_index_to_draw_meshes_info.modify(scene_index);
					draws_info.push_back({node.mesh_index, node.skin_index, transforms[children_index]});
				}
			});
		}
		models.insert(model_id, model_info);
		ids_to_files[model_id] = path;
		files_to_ids[path] = model_id;
		return model_id;
	}

	const Model& Model_3D_System::view_model(uint32_t model_id) {
		if (!models.has(model_id)) {
			throw std::runtime_error(
				("Model_Information::view_model Fail to get model " + std::to_string(model_id)).data()
			);
		}
		return models.get(model_id);
	}

	void Model_3D_System::remove_model(uint32_t model_id) {
		if (!models.has(model_id)) {
			return;
		}
		for (const auto& primitive : models.get(model_id).primitives) {
			static_buffer->remove_data(primitive.vertex_id);
			static_buffer->remove_data(primitive.indices_id);
		}
		models.erase(model_id);
		files_to_ids.erase(ids_to_files[model_id]);
		ids_to_files.erase(model_id);
		id_generator.release_id(model_id);
	}
} // namespace Vulkan