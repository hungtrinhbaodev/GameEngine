#include <math_custom.h>
#include <parser/gltf_parser.h>
#include <vulkan/vk_model_3D_system.h>

namespace Vulkan {

	void Model_3D_System::init(Texture_System* texture_system, Static_Buffer* static_buffer, SSBO_Buffer* ssbo) {
		this->texture_system = texture_system;
		this->static_buffer = static_buffer;
		this->ssbo = ssbo;
	}

	uint32_t Model_3D_System::load_animation(const std::string& path) {
		if (animation_files_to_ids.find(path) != animation_files_to_ids.end()) {
			return animation_files_to_ids[path];
		}
		uint32_t animations_id = animation_id_generator.gen_id();
		Parser::Gltf_Model_Animation_Component animations_config = Parser::parse_gltf_animations_component_by(path);
		Model_Animation_Component animations_info{};
		animations_info.parse_from(animations_config);
		animations.insert(animations_id, std::move(animations_info));
		animation_files_to_ids[path] = animations_id;
		ids_to_animation_files[animations_id] = path;
		return animations_id;
	}

	uint32_t Model_3D_System::load_model(const std::string& path) {
		if (files_to_ids.find(path) != files_to_ids.end()) {
			return files_to_ids[path];
		}
		uint32_t model_id = model_id_generator.gen_id();
		Parser::Gltf_Model model = Parser::parse_gltf_model_by(path);
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
				}
				model_info.primitives.push_back({vertex_id, indices_id, texture_id});
				primitive_indices.push_back(primitive_count);
				primitive_count++;
			}
			model_info.meshes[i] = {std::move(primitive_indices), mesh.min_bounding_box, mesh.max_bounding_box};
		}
		for (const auto& texture : model.textures) {
			if (texture.is_using_bin_texture) {
				delete (texture.data);
			}
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
		models.insert(model_id, std::move(model_info));
		ids_to_files[model_id] = path;
		files_to_ids[path] = model_id;
		return model_id;
	}

	void Model_3D_System::attach_component(uint32_t animations_id, uint32_t to_model_id) {
		/**
		 * @Notes: map from node animations index -> node model index.
		 */
		if (!models.has(to_model_id) || !animations.has(animations_id)) {
			throw std::runtime_error("Fail to attach animation component, model id or animations id not found!");
		}
		auto& model = models.modify(to_model_id);
		if (model.attached_animation_components.find(animations_id) != model.attached_animation_components.end()) {
			return;
		}
		const auto& animations_component = animations.get(animations_id);
		std::vector<int> map_node_indices;
		std::map<std::string, int> model_nodes_name_to_indices;
		for (int i = 0; i < model.nodes.size(); i++) {
			const auto& node = model.nodes[i];
			model_nodes_name_to_indices[node.name] = i;
		}
		/**
		 * Clone all animation from animations component to model.
		 */
		for (const auto& animation : animations_component.animations) {
			Model_Animation model_animation{};
			model_animation.name = animation.name;
			model_animation.samplers = animation.samplers;
			model_animation.channels.resize(animation.channels.size());
			for (int i = 0; i < animation.channels.size(); i++) {
				model_animation.channels[i] = animation.channels[i];
				if (animation.channels[i].node_index >= 0) {
					const auto& animation_node = animations_component.nodes[animation.channels[i].node_index];
					if (model_nodes_name_to_indices.find(animation_node.name) == model_nodes_name_to_indices.end()) {
						throw std::runtime_error(
							"Fail to attached animations component to model, can't map node by name!"
						);
					}
					model_animation.channels[i].node_index = model_nodes_name_to_indices[animation_node.name];
				}
			}
			model_animation.nodes_animation_factors.resize(model.nodes.size());
			for (int i = 0; i < animations_component.nodes.size(); i++) {
				const auto& node_name = animations_component.nodes[i].name;
				if (model_nodes_name_to_indices.find(node_name) == model_nodes_name_to_indices.end()) {
					continue;
				}
				int model_node_index = model_nodes_name_to_indices[node_name];
				model_animation.nodes_animation_factors[model_node_index] = animation.nodes_animation_factors[i];
			}
			int animation_index = -1;
			for (int i = 0; i < model.animations.size(); i++) {
				if (model.animations[i].name == model_animation.name) {
					Log::info(
						"Warning duplicate animation: " + model_animation.name +
						", when attach animations component to model!"
					);
					animation_index = i;
					break;
				}
			}
			if (animation_index >= 0) {
				model.animations[animation_index] = model_animation;
			} else {
				model.animations.push_back(model_animation);
			}
		}
		model.attached_animation_components.insert(animations_id);
	}

	const Model& Model_3D_System::view_model(uint32_t model_id) {
		if (!models.has(model_id)) {
			throw std::runtime_error(
				("Model_Information::view_model Fail to get model " + std::to_string(model_id)).data()
			);
		}
		return models.get(model_id);
	}

	const Model_Animation_Component& Model_3D_System::view_animation(uint32_t animations_id) {
		if (!animations.has(animations_id)) {
			throw std::runtime_error(
				"Model_Information::view_animation Fail to get animation component " + std::to_string(animations_id)
			);
		}
		return animations.get(animations_id);
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
		model_id_generator.release_id(model_id);
	}
} // namespace Vulkan