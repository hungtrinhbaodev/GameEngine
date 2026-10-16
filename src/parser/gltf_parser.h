#pragma once
#include <geometry_structs.h>
#include <glm/gtc/quaternion.hpp>
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
		glm::vec3 translation{0.f, 0.f, 0.f};
		glm::quat rotation{1.f, 0.f, 0.f, 0.f};
		glm::vec3 scale{1.f, 1.f, 1.f};
		glm::mat4 get_transform() const;
		inline friend std::ostream& operator<<(std::ostream& os, const Gltf_Node& node) {
			os << "Parser::Node: {Node name: " << node.name << ", transform: " << node.transform
			   << ", children: " << node.children_indices << ", mesh index: " << node.mesh_index << "}";
			return os;
		}
	};

	struct Gltf_Scene {
		std::vector<uint32_t> node_indices;
		inline friend std::ostream& operator<<(std::ostream& os, const Gltf_Scene& scene) {
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
		glm::vec3 max_bounding_box{-std::numeric_limits<float>::max()};
		glm::vec3 min_bounding_box{std::numeric_limits<float>::max()};
		inline friend std::ostream& operator<<(std::ostream& os, const Gltf_Mesh& mesh) {
			os << "Parser::Mesh {max_bounding_box: " << mesh.max_bounding_box << ", min_bounding_box "
			   << mesh.min_bounding_box << "}";
			return os;
		}
	};

	struct Gltf_Texture {
		bool is_using_bin_texture = false;
		std::string path = "";
		uint8_t* data;
		uint32_t width;
		uint32_t height;
	};

	struct Gltf_Model {
		std::vector<Gltf_Mesh> meshes;
		std::vector<Gltf_Node> nodes;
		std::vector<Gltf_Scene> scenes;
		std::vector<Gltf_Material> materials;
		std::vector<std::string> texture_paths;
		std::vector<Gltf_Texture> textures;
		int default_scene_index = -1;
		std::unordered_map<uint32_t, std::unordered_map<uint32_t, std::vector<glm::mat4>>> mesh_global_transform{};
		/**
		 * scene index -> mesh index -> global transform of mesh
		 */
		std::unordered_map<uint32_t, std::unordered_map<uint32_t, std::vector<glm::mat4>>>
		make_meshes_global_transform();
		std::string get_primitive_texture_path(uint32_t mesh_index, uint32_t primitive_index);
		Gltf_Texture get_primitive_texture(uint32_t mesh_index, uint32_t primitive_index);
		void log_mesh(uint32_t mesh_index);
	};

	Gltf_Model parse_gltf_model(const std::string& path, bool log_debug = false);

} // namespace Parser