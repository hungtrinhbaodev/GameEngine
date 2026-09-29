#include <parser/gltf_parser.h>
#include <vulkan/vk_model_3D_system.h>

namespace Vulkan {

	std::string Model_3D_System::DEFAULT_TEXTURE_PATH = "Model_3D_System::DEFAULT_TEXTURE_PATH::KEY";

	uint32_t Model_3D_System::default_texture_id = -1;

	glm::vec3 Model_Mesh_Information::get_mesh_origin() const {
		return (this->max_bounding_box + this->min_bounding_box) * 0.5f;
	}

	void Model_3D_System::init(
		Static_Buffer* global_vertex_buffer, Static_Buffer* global_indices_buffer, Texture_System* texture_system
	) {
		this->global_vertex_buffer = global_vertex_buffer;
		this->global_indices_buffer = global_indices_buffer;
		this->texture_system = texture_system;
		/**
		 * Make a default texture full white 1x1 pixel.
		 */
		uint8_t default_texture_bytes[4] = {255, 255, 255, 255};
		this->default_texture_id =
			this->texture_system->load_texture(DEFAULT_TEXTURE_PATH, default_texture_bytes, 1, 1, 4);
	}

	uint32_t Model_3D_System::load_model(std::string path) {
		if (this->files_to_ids.find(path) != this->files_to_ids.end()) {
			return this->files_to_ids[path];
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
		for (int mesh_index = 0; mesh_index < model.meshes.size(); mesh_index++) {
			Parser::Gltf_Mesh& mesh = model.meshes[mesh_index];
			std::vector<Primitive_Buffer_Range> primitives;
			for (int primitive_index = 0; primitive_index < mesh.primitives.size(); primitive_index++) {
				Parser::Gltf_Primitive& primitive = mesh.primitives[primitive_index];
				uint32_t vertex_id = this->global_vertex_buffer->upload_data(
					sizeof(Geometry::Vertex_3D) * primitive.vertices.size(), primitive.vertices.data()
				);
				uint32_t indices_id = this->global_indices_buffer->upload_data(
					sizeof(uint32_t) * primitive.indices.size(), primitive.indices.data()
				);
				std::string texture = model.get_primitive_texture_path(mesh_index, primitive_index);
				uint32_t texture_id = this->default_texture_id;
				if (texture != "") {
					texture_id = this->texture_system->load_texture(texture);
				}
				primitives.push_back({vertex_id, indices_id, texture_id});
			}
			model_info.meshes.push_back({std::move(primitives), mesh.min_bounding_box, mesh.max_bounding_box});
		}
		model_info.global_meshes_transform = std::move(model.make_meshes_global_transform());
		model_info.draw_scene_index = model.default_scene_index;
		this->models[model_id] = model_info;
		this->ids_to_files[model_id] = path;
		this->files_to_ids[path] = model_id;
		return model_id;
	}

	const Model_Information& Model_3D_System::view_model(uint32_t model_id) {
		if (this->models.find(model_id) == models.end()) {
			throw std::runtime_error(
				("Model_Information::view_model Fail to get model " + std::to_string(model_id)).data()
			);
		}
		return models[model_id];
	}

	void Model_3D_System::remove_model(uint32_t model_id) {
		if (this->models.find(model_id) == this->models.end()) {
			return;
		}
		Model_Information& model_info = models[model_id];
		for (auto& mesh : model_info.meshes) {
			for (auto& primitive : mesh.primitives) {
				this->global_vertex_buffer->remove_data(primitive.vertex_id);
				this->global_indices_buffer->remove_data(primitive.indices_id);
			}
		}
		this->models.erase(model_id);
		this->files_to_ids.erase(this->ids_to_files[model_id]);
		this->ids_to_files.erase(model_id);
	}
} // namespace Vulkan