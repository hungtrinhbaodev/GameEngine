#pragma once
#include <geometry_structs.h>
#include <log.h>
#include <map>
#include <string>
#include <vector>

namespace Parser {

	using namespace Geometry;

	struct Gltf_Node {
		std::string name = "";
		glm::mat4 transform{};
		std::vector<uint32_t> children_indices;
		int mesh_index = -1;
		friend std::ostream& operator<<(std::ostream& os, const Gltf_Node& node) {
			os << "Parser::Node: {Node name: " << node.name << ", transform: " << node.transform
			   << ", children: " << node.children_indices << ", mesh index: " << node.mesh_index << "}";
			return os;
		}
	};

	struct Gltf_Scene {
		std::vector<uint32_t> node_indices;
		friend std::ostream& operator<<(std::ostream& os, const Gltf_Scene& scene) {
			os << "Parser::Scene {Node indices: " << scene.node_indices << "}";
			return os;
		}
	};

	struct Gltf_Material {
		int texture_index = -1;
		glm::vec4 base_color_factor{1.f};
	};

	struct Gltf_Primitive {
		std::vector<Vertex_3D> vertices;
		std::vector<uint32_t> indices;
		int material_index = -1;
	};

	struct Gltf_Mesh {
		std::vector<Gltf_Primitive> primitives;
		glm::vec3 max_bounding_box{0.f};
		glm::vec3 min_bounding_box{0.f};
	};

	struct Gltf_Model {
		std::vector<Gltf_Mesh> meshes;
		std::vector<Gltf_Node> nodes;
		std::vector<Gltf_Scene> scenes;
		std::vector<Gltf_Material> materials;
		std::vector<std::string> textures;
		int default_scene_index = -1;
		/**
		 * scene index -> mesh index -> global transform of mesh
		 */
		std::map<uint32_t, std::map<uint32_t, std::vector<glm::mat4>>> make_meshes_global_transform();
		std::string get_primitive_texture_path(uint32_t mesh_index, uint32_t primitive_index);
	};

	Gltf_Model parse_gltf_model(const std::string& path, bool log_debug = false);

} // namespace Parser