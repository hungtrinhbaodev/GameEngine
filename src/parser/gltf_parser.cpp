#include <files.h>
#include <json.hpp>
#include <math_custom.h>
#include <parser/gltf_parser.h>
#include <stack>
#include <stb_image.h>
#include <utils.h>

using json = nlohmann::json;

const bool DEBUG_MODEL = true;
const uint32_t CHUNK_TYPE_JSON = 0x4E4F534A;
const uint32_t CHUNK_TYPE_BIN = 0x004E4942;

namespace Parser {

	glm::mat4 Gltf_Node::get_transform() const {
		return (Math::make_translation(translation) * glm::mat4_cast(rotation) * Math::make_scale(scale) * transform);
	}

	std::string Gltf_Model::get_primitive_texture_path(uint32_t mesh_index, uint32_t primitive_index) {
		if (mesh_index < 0 || mesh_index >= meshes.size()) {
			return "";
		}
		Gltf_Mesh& mesh = meshes[mesh_index];
		if (primitive_index < 0 || primitive_index >= mesh.primitives.size()) {
			return "";
		}
		int material_index = mesh.primitives[primitive_index].material_index;
		if (material_index < 0 || material_index >= materials.size()) {
			return "";
		}
		Gltf_Material& material = materials[material_index];
		int texture_index = material.texture_index;
		if (texture_index < 0 || texture_index >= texture_paths.size()) {
			return "";
		}
		return texture_paths[texture_index];
	}

	Gltf_Texture Gltf_Model::get_primitive_texture(uint32_t mesh_index, uint32_t primitive_index) {
		if (mesh_index < 0 || mesh_index >= meshes.size()) {
			return {};
		}
		Gltf_Mesh& mesh = meshes[mesh_index];
		if (primitive_index < 0 || primitive_index >= mesh.primitives.size()) {
			return {};
		}
		int material_index = mesh.primitives[primitive_index].material_index;
		if (material_index < 0 || material_index >= materials.size()) {
			return {};
		}
		Gltf_Material& material = materials[material_index];
		int texture_index = material.texture_index;
		if (texture_index < 0 || texture_index >= texture_paths.size()) {
			return {};
		}
		return textures[texture_index];
	}

	glm::vec3 Gltf_Model::get_bounding_box() const {
		glm::vec3 min = glm::vec3{std::numeric_limits<float>::max()};
		glm::vec3 max = glm::vec3{-std::numeric_limits<float>::max()};
		for (const auto& mesh : meshes) {
			min = glm::min(min, mesh.min_bounding_box);
			max = glm::max(max, mesh.max_bounding_box);
		}
		return (max + min) * 0.5f;
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

	glm::vec2 read_vec2_in_bin_by_component_type(
		const uint8_t* bin_data, int index, int component_type, glm::vec2 default_result
	) {
		glm::vec2 result{};
		if (component_type == 5126) {
			const float* data = (const float*)bin_data + index * 2;
			result = glm::vec2(data[0], data[1]);
		} else if (component_type == 5121) {
			const uint8_t* data = bin_data + index * 2;
			result = glm::vec2(data[0] / 255.f, data[1] / 255.f);
		} else if (component_type == 5123) {
			const uint16_t* data = (const uint16_t*)bin_data + index * 2;
			result = glm::vec2(data[0] / 65535.f, data[1] / 65535.f);
		} else {
			return default_result;
		}
		return result;
	}

	glm::vec3 read_vec3_in_bin_by_component_type(
		const uint8_t* bin_data, int index, int component_type, glm::vec3 default_result
	) {
		glm::vec3 result{};
		if (component_type == 5126) {
			const float* data = (const float*)bin_data + index * 3;
			result = glm::vec3(data[0], data[1], data[2]);
		} else if (component_type == 5121) {
			const uint8_t* data = bin_data + index * 3;
			result = glm::vec3(data[0] / 255.f, data[1] / 255.f, data[2] / 255.f);
		} else if (component_type == 5123) {
			const uint16_t* data = (const uint16_t*)bin_data + index * 3;
			result = glm::vec3(data[0] / 65535.f, data[1] / 65535.f, data[2] / 65535.f);
		} else {
			return default_result;
		}
		return result;
	}

	glm::vec4 read_vec4_in_bin_by_component_type(
		const uint8_t* bin_data, int index, int component_type, glm::vec4 default_result
	) {
		glm::vec4 result{};
		if (component_type == 5126) {
			const float* data = (const float*)bin_data + index * 4;
			result = glm::vec4(data[0], data[1], data[2], data[3]);
		} else if (component_type == 5121) {
			const uint8_t* data = bin_data + index * 4;
			result = glm::vec4(data[0] / 255.f, data[1] / 255.f, data[2] / 255.f, data[3] / 255.f);
		} else if (component_type == 5123) {
			const uint16_t* data = (const uint16_t*)bin_data + index * 4;
			result = glm::vec4(data[0] / 65535.f, data[1] / 65535.f, data[2] / 65535.f, data[3] / 65535.f);
		} else {
			return default_result;
		}
		return result;
	}

	glm::quat read_quat_in_bin_by_component_type(
		const uint8_t* bin_data, int index, int component_type, glm::quat default_result
	) {
		glm::quat result{};
		if (component_type == 5126) {
			const float* data = (const float*)bin_data + index * 4;
			result = glm::quat(data[3], data[0], data[1], data[2]);
		} else if (component_type == 5121) {
			const uint8_t* data = bin_data + index * 4;
			result = glm::quat(data[3] / 255.f, data[0] / 255.f, data[1] / 255.f, data[2] / 255.f);
		} else if (component_type == 5123) {
			const uint16_t* data = (const uint16_t*)bin_data + index * 4;
			result = glm::quat(data[3] / 65535.f, data[0] / 65535.f, data[1] / 65535.f, data[2] / 65535.f);
		} else {
			return default_result;
		}
		return result;
	}

	Gltf_Model parse_gltf_model(const std::string& path) {
		json root;
		const uint8_t* bin_data = nullptr;
		std::vector<char> file_bytes;
		std::vector<std::vector<char>> file_bin_buffers;
		std::vector<const uint8_t*> buffers;
		file_bytes = Files::read_file(Utils::get_root_path() + path);
		if (path.find(".glb") != std::string::npos) {
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
				const std::string& uri = buffers_info[i]["uri"].get_ref<const std::string&>();
				size_t quate_index = uri.find(",");
				if (quate_index != std::string::npos) {
					auto quate_index = uri.find(",");
					std::string_view data = std::string_view(uri).substr(quate_index + 1);
					file_bin_buffers[i] = Utils::base64_to_bytes(data);
				} else {
					file_bin_buffers[i] = Files::read_file(Utils::get_folder_path_from(path) + uri);
				}
				buffers[i] = (const uint8_t*)file_bin_buffers[i].data();
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

		auto get_bin_data_at = [&root, get_bin_data](const json& attributes, const char* attribute_name) {
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
							glm::vec3 prim_max = glm::make_vec_from<3>(flat_max_bounding_box);
							mesh.max_bounding_box = glm::max(prim_max, mesh.max_bounding_box);
						}
						if (pos_accessor.contains("min")) {
							std::vector<float> flat_min_bounding_box = pos_accessor["min"].get<std::vector<float>>();
							glm::vec3 prim_min = glm::make_vec_from<3>(flat_min_bounding_box);
							mesh.min_bounding_box = glm::min(prim_min, mesh.min_bounding_box);
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
								primitive.vertices[i].tex_coord =
									read_vec2_in_bin_by_component_type(tex_coords_data, i, component_type, {0.f, 0.f});
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
					/**
					 * Parse Color
					 */
					{
						if (attributes.contains("COLOR_0")) {
							auto& acc = root["accessors"][(int)attributes["COLOR_0"]];
							std::string type = acc["type"];
							int component_type = acc["componentType"];
							const uint8_t* color_data = get_bin_data_at(attributes, "COLOR_0");

							for (uint32_t i = 0; i < primitive.vertices.size(); i++) {
								primitive.vertices[i].color =
									read_vec3_in_bin_by_component_type(color_data, i, component_type, {1.f, 1.f, 1.f});
							}
						}
					}
					/**
					 * Parse joints.
					 */
					{
						std::array<std::string, 2> joints_keys = {"JOINTS_0", "JOINTS_1"};
						std::string using_key = "";
						for (int i = 0; i < joints_keys.size(); i++) {
							if (attributes.contains(joints_keys[i].data())) {
								using_key = joints_keys[i];
							}
						}
						if (attributes.contains(using_key.data())) {
							auto& acc = root["accessors"][(int)attributes[using_key.data()]];
							std::string type = acc["type"];
							const uint8_t* joints_data = get_bin_data_at(attributes, using_key.data());
							int component_type = acc["componentType"];
							for (int i = 0; i < primitive.vertices.size(); i++) {
								glm::u16vec4 joint{0};
								if (component_type == 5121) {
									const uint8_t* joints = joints_data + i * 4;
									joint = {joints[0], joints[1], joints[2], joints[3]};
								} else if (component_type == 5123) {
									const uint16_t* joints = (const uint16_t*)joints_data + i * 4;
									joint = {joints[0], joints[1], joints[2], joints[3]};
								}
								primitive.vertices[i].joints = joint;
							}
						}
					}
					/**
					 * Parse weights.
					 */
					{
						std::array<std::string, 2> weights_keys = {"WEIGHTS_0", "WEIGHTS_1"};
						std::string using_key = "";
						for (int i = 0; i < weights_keys.size(); i++) {
							if (attributes.contains(weights_keys[i].data())) {
								using_key = weights_keys[i];
							}
						}
						if (attributes.contains(using_key.data())) {
							auto& acc = root["accessors"][(int)attributes[using_key.data()]];
							std::string type = acc["type"];
							const uint8_t* weights_data = get_bin_data_at(attributes, using_key.data());
							int component_type = acc["componentType"];
							for (int i = 0; i < primitive.vertices.size(); i++) {
								primitive.vertices[i].weights = read_vec4_in_bin_by_component_type(
									weights_data, i, component_type, {0.f, 0.f, 0.f, 0.f}
								);
							}
						}
					}
					/**
					 * Parse indices.
					 */
					{
						if (primitive_json.contains("indices")) {
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
						} else {
							primitive.indices.resize(primitive.vertices.size());
							for (int i = 0; i < primitive.indices.size(); i++) {
								primitive.indices[i] = i;
							}
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
					if (DEBUG_MODEL) {
						// Log::info("What is my vertices data", mesh.min_bounding_box, mesh.max_bounding_box);
					}
					primitive.material_index = primitive_json.value("material", -1);
					mesh.primitives.push_back(std::move(primitive));
				}
				model.meshes.push_back(std::move(mesh));
			}
		}
		/**
		 * Parse skin.
		 */
		{
			if (root.contains("skins")) {
				auto& json_skins = root["skins"];
				model.skins.resize(json_skins.size());
				for (int i = 0; i < model.skins.size(); i++) {
					auto& json_skin = json_skins[i];
					const uint8_t* matrices_data = get_bin_data_at(json_skin, "inverseBindMatrices");
					Gltf_Skin skin{};
					skin.joints = json_skin["joints"].get<std::vector<uint32_t>>();
					skin.inverse_bind_poses.resize(skin.joints.size());
					for (int j = 0; j < skin.inverse_bind_poses.size(); j++) {
						skin.inverse_bind_poses[j] = glm::make_mat4_from((const float*)matrices_data + j * 16);
					}
					model.skins[i] = skin;
				}
			}
		}
		/**
		 * Parse scene, node.
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
					node.children_indices = json_node["children"].get<std::vector<int>>();
				}
				if (json_node.contains("name")) {
					node.name = json_node.value("name", "");
				}
				if (json_node.contains("matrix")) {
					std::vector<float> cols = json_node["matrix"].get<std::vector<float>>();
					node.transform = glm::make_mat4_from(cols);
					node.is_use_matrix = true;
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
						rotation = glm::quat(flat_rotation[3], flat_rotation[0], flat_rotation[1], flat_rotation[2]);
					}
					node.transform = {1.f};
					node.translation = translation;
					node.scale = scale;
					node.rotation = rotation;
				}
				if (json_node.contains("mesh")) {
					node.mesh_index = json_node.value("mesh", 0);
				}
				if (json_node.contains("skin")) {
					node.skin_index = json_node.value("skin", -1);
				}
			}
			if (DEBUG_MODEL) {
				// Log::info("What is my nodes data", nodes.size(), nodes);
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
					scenes[i].node_indices = json_scenes[i]["nodes"].get<std::vector<uint32_t>>();
					if (json_scenes[i].contains("name")) {
						scenes[i].name = json_scenes[i]["name"];
					} else {
						scenes[i].name = std::to_string(i);
					}
				}
			}
			if (DEBUG_MODEL) {
				// Log::info("what is my scenes", model.default_scene_index, scenes);
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
				model.texture_paths.resize(json_images.size());
				model.textures.resize(json_images.size());
				for (int i = 0; i < model.texture_paths.size(); i++) {
					if (json_images[i].contains("uri")) {
						const std::string& uri = json_images[i]["uri"].get_ref<const std::string&>();
						if (uri.find("data:") == 0) {
							std::vector<char> bytes =
								Utils::base64_to_bytes(std::string_view(uri).substr(uri.find(',') + 1));
							const uint8_t* image_bytes = (const uint8_t*)bytes.data();
							uint32_t byte_length = bytes.size();
							int width = 0;
							int height = 0;
							stbi_uc* pixels = stbi_load_from_memory(
								image_bytes, byte_length, &width, &height, nullptr, STBI_rgb_alpha
							);
							if (pixels == nullptr) {
								throw std::runtime_error("Fail to load texture in uri data!");
							}
							model.textures[i] = {
								true, path + std::to_string(i), (uint8_t*)pixels, (uint32_t)width, (uint32_t)height
							};
						} else {
							model.texture_paths[i] =
								Utils::get_folder_path_from(path) + json_images[i].value("uri", "");
							model.textures[i] = {
								false, Utils::get_folder_path_from(path) + json_images[i].value("uri", "")
							};
						}
					} else if (json_images[i].contains("bufferView")) {
						int bv_index = json_images[i]["bufferView"];
						auto& bv = root["bufferViews"][bv_index];
						int buffer_index = bv.value("buffer", 0);
						const uint8_t* image_bytes = get_bin_data(buffer_index) + (uint32_t)bv.value("byteOffset", 0);
						uint32_t byte_length = bv["byteLength"];
						int width = 0;
						int height = 0;
						stbi_uc* pixels =
							stbi_load_from_memory(image_bytes, byte_length, &width, &height, nullptr, STBI_rgb_alpha);
						if (pixels == nullptr) {
							throw std::runtime_error("Fail to load texture in bin data!");
						}
						model.textures[i] = {
							true, path + std::to_string(i), (uint8_t*)pixels, (uint32_t)width, (uint32_t)height
						};
					} else {
						throw std::runtime_error("Fail to load texture model!");
					}
				}
			}
		}
		/**
		 * Parse animations.
		 */
		{
			if (root.contains("animations")) {
				auto& json_animations = root["animations"];
				model.animations.resize(json_animations.size());
				for (int i = 0; i < model.animations.size(); i++) {
					auto& json_animation = json_animations[i];
					auto& animation = model.animations[i];
					if (json_animation.contains("name")) {
						animation.name = json_animation["name"];
					} else {
						animation.name = std::to_string(i);
					}
					if (json_animation.contains("channels")) {
						auto& json_chanels = json_animation["channels"];
						auto& channels = animation.channels;
						channels.resize(json_chanels.size());
						for (int j = 0; j < channels.size(); j++) {
							auto& json_channel = json_chanels[j];
							auto& channel = channels[j];
							channel.sampler_index = json_channel["sampler"];
							if (json_channel.contains("target")) {
								const auto& json_target = json_channel["target"];
								if (json_target.contains("path")) {
									channel.type_sampler = json_target["path"];
								}
								if (json_target.contains("node")) {
									channel.use_node_index = true;
									channel.node_index = json_target["node"];
								}
							}
						}
						if (DEBUG_MODEL) {
							// Log::info("What is my channels", channels);
						}
					}
					if (json_animation.contains("samplers")) {
						const auto& json_samplers = json_animation["samplers"];
						auto& samplers = animation.samplers;
						samplers.resize(json_samplers.size());
						for (int j = 0; j < samplers.size(); j++) {
							const auto& json_sampler = json_samplers[j];
							auto& sampler = samplers[j];
							if (json_sampler.contains("interpolation")) {
								sampler.interpolation = json_sampler["interpolation"];
							} else {
								sampler.interpolation = "LINEAR";
							}
							const auto& duration_accessor = root["accessors"][(int)json_sampler["input"]];
							const float* samplers_input_data = (const float*)get_bin_data_at(json_sampler, "input");
							auto& durations = sampler.durations;
							durations.resize((int)duration_accessor["count"]);
							for (int k = 0; k < durations.size(); k++) {
								durations[k] = samplers_input_data[k];
							}
							const auto& data_accessor = root["accessors"][(int)json_sampler["output"]];
							const uint8_t* samplers_output_data = get_bin_data_at(json_sampler, "output");
							int component_type = data_accessor["componentType"];
							if (component_count_from_type(data_accessor["type"]) == 3) {
								auto& vec3s = sampler.vec3s;
								vec3s.resize(data_accessor["count"]);
								for (int k = 0; k < vec3s.size(); k++) {
									vec3s[k] = read_vec3_in_bin_by_component_type(
										samplers_output_data, k, component_type, {0.f, 0.f, 0.f}
									);
								}
								if (data_accessor.contains("min")) {
									sampler.min_vec3 =
										glm::make_vec_from<3>(data_accessor["min"].get<std::vector<float>>());
								}
								if (data_accessor.contains("max")) {
									sampler.max_vec3 =
										glm::make_vec_from<3>(data_accessor["max"].get<std::vector<float>>());
								}
							} else if (component_count_from_type(data_accessor["type"]) == 4) {
								auto& quats = sampler.quats;
								quats.resize(data_accessor["count"]);
								for (int k = 0; k < quats.size(); k++) {
									quats[k] = read_quat_in_bin_by_component_type(
										samplers_output_data, k, component_type, {1.f, 0.f, 0.f, 0.f}
									);
								}
								if (data_accessor.contains("min")) {
									sampler.min_quat = read_quat_in_bin_by_component_type(
										(const uint8_t*)data_accessor["min"].get<std::vector<float>>().data(), 0,
										component_type, {1.f, 0.f, 0.f, 0.f}
									);
								}
								if (data_accessor.contains("max")) {
									sampler.max_quat = read_quat_in_bin_by_component_type(
										(const uint8_t*)data_accessor["max"].get<std::vector<float>>().data(), 0,
										component_type, {1.f, 0.f, 0.f, 0.f}
									);
								}
							}
							if (DEBUG_MODEL) {
								// Log::info("What is sampler", j, sampler);
							}
						}
					}
				}
			}
		}
		if (DEBUG_MODEL) {
			// std::vector<std::string> animation_names{};
			// for (const auto& animation : model.animations) {
			// 	animation_names.push_back(animation.name);
			// }
			// Log::info("Model information", path, animation_names);
		}
		return model;
	}

} // namespace Parser
