#include <files.h>
#include <glm/gtc/quaternion.hpp>
#include <json.hpp>
#include <math_custom.h>
#include <parser/gltf_parser.h>
#include <stack>
#include <utils.h>

using json = nlohmann::json;

const uint32_t CHUNK_TYPE_JSON = 0x4E4F534A;
const uint32_t CHUNK_TYPE_BIN = 0x004E4942;

namespace Parser {

	std::map<uint32_t, std::map<uint32_t, std::vector<glm::mat4>>> Gltf_Model::make_meshes_global_transform() {
		std::map<uint32_t, std::map<uint32_t, std::vector<glm::mat4>>> meshes_transform{};
		for (int i = 0; i < scenes.size(); i++) {
			meshes_transform[i] = {};
		}
		const std::vector<Gltf_Node>& nodes = this->nodes;
		std::vector<bool> visited(nodes.size(), false);
		std::vector<glm::mat4> global_transforms(nodes.size(), {1.f});
		std::stack<uint32_t> stack{};
		auto add_transform_to_mesh_at_scene = [&global_transforms, &meshes_transform, &nodes](
												  uint32_t scene_index, uint32_t node_index, glm::mat4& transform
											  ) {
			const Gltf_Node& node = nodes[node_index];
			if (node.mesh_index != -1) {
				if (meshes_transform[scene_index].find(node.mesh_index) == meshes_transform[scene_index].end()) {
					meshes_transform[scene_index][node.mesh_index] = {};
				}
				meshes_transform[scene_index][node.mesh_index].push_back(global_transforms[node_index]);
			}
		};
		for (int i = 0; i < scenes.size(); i++) {
			std::vector<uint32_t>& node_indices = scenes[i].node_indices;
			for (int root : node_indices) {
				for (int j = 0; j < nodes.size(); j++) {
					global_transforms[j] = nodes[j].transform;
					visited[j] = false;
				}

				stack.push(root);
				visited[root] = true;
				add_transform_to_mesh_at_scene(i, root, global_transforms[root]);

				while (stack.size() > 0) {
					int current = stack.top();
					const glm::mat4& current_global_transform = global_transforms[current];
					stack.pop();
					const std::vector<uint32_t>& children_indices = nodes[current].children_indices;
					for (int k = 0; k < children_indices.size(); k++) {
						int child = children_indices[k];
						if (!visited[child]) {
							global_transforms[child] = current_global_transform * global_transforms[child];
							add_transform_to_mesh_at_scene(i, child, global_transforms[child]);
							visited[child] = true;
							stack.push(child);
						}
					}
				}
			}
		}
		return meshes_transform;
	}

	std::string Gltf_Model::get_primitive_texture_path(uint32_t mesh_index, uint32_t primitive_index) {
		if (mesh_index < 0 || mesh_index >= this->meshes.size()) {
			return "";
		}
		Gltf_Mesh& mesh = this->meshes[mesh_index];
		if (primitive_index < 0 || primitive_index >= mesh.primitives.size()) {
			return "";
		}
		int material_index = mesh.primitives[primitive_index].material_index;
		if (material_index < 0 || material_index >= this->materials.size()) {
			return "";
		}
		Gltf_Material& material = this->materials[material_index];
		int texture_index = material.texture_index;
		if (texture_index < 0 || texture_index >= this->textures.size()) {
			return "";
		}
		return this->textures[texture_index];
	}

	int component_count_from_type(const std::string& type) {
		if (type == "SCALAR")
			return 1;
		if (type == "VEC2")
			return 2;
		if (type == "VEC3")
			return 3;
		if (type == "VEC4")
			return 4;
		return 0;
	}

	Gltf_Model parse_gltf_model(const std::string& path, bool log_debug) {
		json root;
		const uint8_t* bin_data = nullptr;
		std::vector<char> file_bytes;
		std::vector<std::vector<char>> file_bin_buffers;
		std::vector<const uint8_t*> buffers;
		file_bytes = Files::read_file(Utils::get_root_path() + path);
		if (path.find(".glb") != std::string::npos) {
			std::vector<char> magic = Utils::parse_char<char>(file_bytes, 4, 0);
			uint32_t version = Utils::parse_uint32_t(file_bytes.data(), 4);
			uint32_t total_file_length = Utils::parse_uint32_t(file_bytes.data(), 8);
			uint32_t chunk_offset = 12;
			while (chunk_offset < total_file_length) {
				uint32_t chunk_length = Utils::parse_uint32_t(file_bytes.data(), chunk_offset);
				uint32_t chunk_type = Utils::parse_uint32_t(file_bytes.data(), chunk_offset + 4);
				if (chunk_type == CHUNK_TYPE_JSON) {
					uint32_t json_offset = chunk_offset + 8;
					const uint8_t* start_json = (uint8_t*)file_bytes.data() + json_offset;
					const uint8_t* end_json = start_json + chunk_length;
					root = json::parse(start_json, end_json);
				} else {
					bin_data = (uint8_t*)file_bytes.data() + chunk_offset + 8;
				}
				chunk_offset = chunk_offset + 8 + chunk_length;
			}
		} else if (path.find(".gltf") != std::string::npos) {
			root = json::parse(file_bytes.begin(), file_bytes.end());
			auto& buffers_info = root["buffers"];
			int buffer_info_size = buffers_info.size();

			buffers.resize(buffer_info_size);
			file_bin_buffers.resize(buffer_info_size);

			for (int i = 0; i < buffer_info_size; i++) {
				int byte_length = buffers_info[i].value("byteLength", 0);
				const std::string& uri = buffers_info[i]["uri"].get_ref<const std::string&>();
				size_t quate_index = uri.find(",");
				if (quate_index != std::string::npos) {
					auto quate_index = uri.find(",");
					buffers[i] = (const uint8_t*)uri.data() + quate_index;
				} else {
					file_bin_buffers[i] = Files::read_file(Utils::get_folder_path_from(path) + uri);
					buffers[i] = (const uint8_t*)file_bin_buffers[i].data();
				}
			}
		} else {
			throw std::runtime_error(("Fail to load model 3D, unsuported model, " + path + "!").data());
		}

		auto get_bin_data = [&buffers, &bin_data, &path](int buffer_index) {
			if (path.find(".glb") != std::string::npos) {
				return bin_data;
			} else {
				return (const uint8_t*)buffers[buffer_index];
			}
		};

		auto get_bin_data_at = [&root, get_bin_data](json& attributes, const char* attribute_name) {
			int pos_accessor_index = attributes[attribute_name];
			auto& pos_accessor = root["accessors"][pos_accessor_index];
			auto& pos_buffer_view = root["bufferViews"][(int)pos_accessor["bufferView"]];
			int buffer_index = pos_buffer_view.value("buffer", 0);
			const uint8_t* data = get_bin_data(buffer_index);
			return (const uint8_t*)(data + (uint32_t)pos_buffer_view["byteOffset"] +
									pos_accessor.value("byteOffset", 0));
		};

		Gltf_Model model{};
		/**
		 * Parse vertices, indices.
		 */
		{
			for (auto& mesh_json : root["meshes"]) {
				Gltf_Mesh mesh{};

				for (auto& primitive_json : mesh_json["primitives"]) {
					Gltf_Primitive primitive{};
					auto& attributes = primitive_json["attributes"];
					/**
					 * Parse position.
					 */
					{
						int pos_accessor_index = attributes["POSITION"];
						auto& pos_accessor = root["accessors"][pos_accessor_index];
						const float* positions = (const float*)get_bin_data_at(attributes, "POSITION");

						uint32_t vertex_count = pos_accessor["count"];
						primitive.vertices.resize(vertex_count);
						for (uint32_t i = 0; i < primitive.vertices.size(); i++) {
							uint32_t base_offset = i * 3;
							primitive.vertices[i].position = glm::vec3(
								positions[base_offset], positions[base_offset + 1], positions[base_offset + 2]
							);
						}

						if (pos_accessor.contains("max")) {
							std::vector<float> flat_max_bounding_box = pos_accessor["max"].get<std::vector<float>>();
							mesh.max_bounding_box = glm::make_vec_from<3>(flat_max_bounding_box);
						}
						if (pos_accessor.contains("min")) {
							std::vector<float> flat_min_bounding_box = pos_accessor["min"].get<std::vector<float>>();
							mesh.min_bounding_box = glm::make_vec_from<3>(flat_min_bounding_box);
						}
					}
					/**
					 * Parse normal.
					 */
					{
						if (attributes.contains("NORMAL")) {
							const float* normals = (const float*)get_bin_data_at(attributes, "NORMAL");
							for (uint32_t i = 0; i < primitive.vertices.size(); i++) {
								uint32_t base_offset = i * 3;
								primitive.vertices[i].normal =
									glm::vec3(normals[base_offset], normals[base_offset + 1], normals[base_offset + 2]);
							}
						}
					}
					/**
					 * Parse Tex coord.
					 */
					{
						if (attributes.contains("TEXCOORD_0")) {
							auto& acc = root["accessors"][(int)attributes["TEXCOORD_0"]];
							int component_type = acc["componentType"];
							const uint8_t* tex_coords_data = get_bin_data_at(attributes, "TEXCOORD_0");

							for (uint32_t i = 0; i < primitive.vertices.size(); i++) {
								glm::vec2 tex_coord{0.f, 0.f};

								if (component_type == 5126) {
									const float* tex_coords = (const float*)tex_coords_data + i * 2;
									tex_coord = glm::vec2(tex_coords[0], tex_coords[1]);
								} else if (component_type == 5121) {
									const uint8_t* tex_coords = tex_coords_data + i * 2;
									tex_coord = glm::vec2(tex_coords[0] / 255.f, tex_coords[1] / 255.f);
								} else if (component_type == 5123) {
									const uint16_t* tex_coords = (const uint16_t*)tex_coords_data + i * 2;
									tex_coord = glm::vec2(tex_coords[0] / 65535.f, tex_coords[1] / 65535.f);
								}

								primitive.vertices[i].tex_coord = tex_coord;
							}
						}
					}
					/**
					 * Parse Tangent
					 */
					{
						if (attributes.contains("TANGENT")) {
							const float* tangents = (const float*)get_bin_data_at(attributes, "TANGENT");
							for (uint32_t i = 0; i < primitive.vertices.size(); i++) {
								uint32_t base_offset = i * 4;
								primitive.vertices[i].tangent = glm::vec4(
									tangents[base_offset], tangents[base_offset + 1], tangents[base_offset + 2],
									tangents[base_offset + 3]
								);
							}
						}
					}
					{
						if (attributes.contains("COLOR_0")) {
							auto& acc = root["accessors"][(int)attributes["COLOR_0"]];
							std::string type = acc["type"];
							int component_type = acc["componentType"];
							int num_components = component_count_from_type(type);
							const uint8_t* color_data = get_bin_data_at(attributes, "COLOR_0");

							for (uint32_t i = 0; i < primitive.vertices.size(); i++) {
								glm::vec3 color{1.f, 1.f, 1.f};

								if (component_type == 5126) {
									const float* colors = (const float*)color_data + i * num_components;
									color = glm::vec3(colors[0], colors[1], colors[2]);
								} else if (component_type == 5121) {
									const uint8_t* colors = color_data + i * num_components;
									color = glm::vec3(colors[0] / 255.f, colors[1] / 255.f, colors[2] / 255.f);
								} else if (component_type == 5123) {
									const uint16_t* colors = (const uint16_t*)color_data + i * num_components;
									color = glm::vec3(colors[0] / 65535.f, colors[1] / 65535.f, colors[2] / 65535.f);
								}
								primitive.vertices[i].color = color;
							}
						}
					}
					/**
					 * Parse indices.
					 */
					{
						int index_accessor_idx = primitive_json["indices"];
						auto& idx_accessor = root["accessors"][index_accessor_idx];
						const uint8_t* idx_data = get_bin_data_at(primitive_json, "indices");

						uint32_t index_count = idx_accessor["count"];
						int component_type = idx_accessor["componentType"];
						primitive.indices.resize(index_count);
						for (uint32_t i = 0; i < index_count; i++) {
							if (component_type == 5121)
								primitive.indices[i] = idx_data[i];
							else if (component_type == 5123)
								primitive.indices[i] = ((const uint16_t*)idx_data)[i];
							else if (component_type == 5125)
								primitive.indices[i] = ((const uint32_t*)idx_data)[i];
						}
					}
					/**
					 * In case model don't have normal we auto generate it!
					 */
					{
						if (!attributes.contains("NORMAL")) {
							const uint32_t NUMBER_TRIANGLE_VERTEX = 3;
							std::vector<Vertex_3D>& vertices = primitive.vertices;
							std::vector<uint32_t>& indices = primitive.indices;
							for (int i = 0; i < primitive.indices.size() / NUMBER_TRIANGLE_VERTEX; i++) {
								uint32_t start = i * NUMBER_TRIANGLE_VERTEX;
								uint32_t& index_a = indices[start];
								uint32_t& index_b = indices[start + 1];
								uint32_t& index_c = indices[start + 2];
								Vertex_3D& a = vertices[index_a];
								Vertex_3D& b = vertices[index_b];
								Vertex_3D& c = vertices[index_c];
								glm::vec3 face_normal =
									glm::normalize(glm::cross(b.position - a.position, c.position - a.position));
								a.normal += face_normal;
								b.normal += face_normal;
								c.normal += face_normal;
							}
							for (int i = 0; i < vertices.size(); i++) {
								Vertex_3D& v = vertices[i];
								v.normal = glm::normalize(v.normal);
							}
						}
					}
					if (log_debug) {
						Log::info(
							"What is my vertices data", primitive.vertices.size(), primitive.vertices[0],
							primitive.indices.size(), primitive.indices[0]
						);
					}
					primitive.material_index = primitive_json.value("material", -1);
					mesh.primitives.push_back(std::move(primitive));
				}
				model.meshes.push_back(std::move(mesh));
			}
		}
		/**
		 * Parse scene, node
		 */
		{
			auto& json_nodes = root["nodes"];
			int node_size = json_nodes.size();
			std::vector<Gltf_Node>& nodes = model.nodes;
			nodes.resize(node_size);
			for (int i = 0; i < node_size; i++) {
				auto& json_node = json_nodes[i];
				Gltf_Node& node = nodes[i];
				if (json_node.contains("children")) {
					auto& children_indices = json_node["children"];
					node.children_indices.resize(children_indices.size());
					for (int j = 0; j < children_indices.size(); j++) {
						uint32_t child = children_indices[j];
						node.children_indices.push_back(child);
					}
				}
				if (json_node.contains("name")) {
					node.name = json_node.value("name", "");
				}
				glm::mat4 transform{};
				if (json_node.contains("matrix")) {
					std::vector<float> cols = json_node["matrix"].get<std::vector<float>>();
					node.transform = glm::make_mat4_from(cols);
				} else {
					std::vector<float> flat_translation{}, flat_rotation{}, flat_scale{};
					if (json_node.contains("translation")) {
						flat_translation = json_node["translation"].get<std::vector<float>>();
					}
					if (json_node.contains("rotation")) {
						flat_rotation = json_node["rotation"].get<std::vector<float>>();
					}
					if (json_node.contains("scale")) {
						flat_scale = json_node["scale"].get<std::vector<float>>();
					}
					glm::vec3 translation{0.f}, scale{1.f};
					glm::quat rotation(1.f, 0.f, 0.f, 0.f);
					if (flat_translation.size() > 0) {
						translation = glm::make_vec_from<3>(flat_translation);
					}
					if (flat_scale.size() > 0) {
						scale = glm::make_vec_from<3>(flat_scale);
					}
					if (flat_rotation.size() > 0) {
						rotation = glm::quat(flat_rotation[0], flat_rotation[1], flat_rotation[2], flat_rotation[3]);
					}
					node.transform = {1.f};
					node.transform *=
						(Math::make_translation(translation) * glm::mat4_cast(rotation) * Math::make_scale(scale));
				}
				if (json_node.contains("mesh")) {
					node.mesh_index = json_node.value("mesh", 0);
				}
			}
			if (log_debug) {
				Log::info("What is my nodes data", nodes.size(), nodes);
			}
			/**
			 * Parse scene.
			 */
			std::vector<Gltf_Scene>& scenes = model.scenes;
			if (root.contains("scene")) {
				model.default_scene_index = root.value("scene", 0);
			}
			if (root.contains("scenes")) {
				auto& json_scenes = root["scenes"];
				int scene_size = json_scenes.size();
				scenes.resize(scene_size);
				for (int i = 0; i < scene_size; i++) {
					scenes[i] = {json_scenes[i]["nodes"].get<std::vector<uint32_t>>()};
				}
			}
			if (log_debug) {
				Log::info("what is my scenes", model.default_scene_index, scenes);
			}
		}
		/**
		 * Parse meterial data.
		 */
		{
			if (root.contains("materials")) {
				auto& json_materials = root["materials"];
				model.materials.resize(json_materials.size());
				for (int i = 0; i < model.materials.size(); i++) {
					auto json_material = json_materials[i];
					if (json_material.contains("pbrMetallicRoughness")) {
						auto& json_pbr = json_material["pbrMetallicRoughness"];
						if (json_pbr.contains("baseColorTexture")) {
							auto& json_color_texture = json_pbr["baseColorTexture"];
							model.materials[i].texture_index = json_color_texture.value("index", -1);
						}
					}
					if (json_material.contains("baseColorFactor")) {
						std::vector<float> flat_color = json_material["baseColorFactor"].get<std::vector<float>>();
						model.materials[i].base_color_factor = glm::make_vec_from<4>(flat_color);
					}
				}
			}
		}
		/**
		 * Parse textures path.
		 */
		{
			if (root.contains("images")) {
				auto& json_images = root["images"];
				model.textures.resize(json_images.size());
				for (int i = 0; i < model.textures.size(); i++) {
					model.textures[i] = Utils::get_folder_path_from(path) + json_images[i].value("uri", "");
				}
			}
		}
		return model;
	}

} // namespace Parser
