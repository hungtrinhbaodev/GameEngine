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
		std::vector<int> children_indices;
		int mesh_index = -1;
		int skin_index = -1;
		glm::vec3 translation{0.f, 0.f, 0.f};
		glm::quat rotation{1.f, 0.f, 0.f, 0.f};
		glm::vec3 scale{1.f, 1.f, 1.f};
		bool is_use_matrix = false;
		glm::mat4 get_transform() const;
		inline friend std::ostream& operator<<(std::ostream& os, const Gltf_Node& node) {
			os << "Parser::Node: {Node name: " << node.name << ", transform: " << node.transform
			   << ", children: " << node.children_indices << ", mesh index: " << node.mesh_index << "}";
			return os;
		}
	};

	struct Gltf_Scene {
		std::vector<uint32_t> node_indices;
		std::string name = "";
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

	struct Gltf_Channel {
		bool use_node_index = false;
		uint32_t sampler_index = 0;
		std::string type_sampler = "";
		uint32_t node_index = -1;
		inline friend std::ostream& operator<<(std::ostream& os, const Gltf_Channel& channel) {
			os << "Parser::Gltf_Channel{sampler_index: " << channel.sampler_index
			   << ", use_node_index: " << channel.use_node_index << ", type_sampler: " << channel.type_sampler
			   << ", node_index: " << channel.node_index << "}";
			return os;
		}
	};

	struct Gltf_Sampler {
		uint32_t number_apply = 0;
		std::string interpolation = "";
		std::vector<float> durations{};
		std::vector<glm::vec3> vec3s{};
		std::vector<glm::quat> quats{};
		glm::vec3 min_vec3{0};
		glm::vec3 max_vec3{0};
		glm::quat min_quat{1.f, 0.f, 0.f, 0.f};
		glm::quat max_quat{1.f, 0.f, 0.f, 0.f};
		inline friend std::ostream& operator<<(std::ostream& os, const Gltf_Sampler& sampler) {
			os << "Parser::Gltf_Sampler{number_apply: " << sampler.number_apply
			   << ", interpolation: " << sampler.interpolation << ", durations: " << sampler.durations
			   << ", vec3s: " << sampler.vec3s << ", quats: " << sampler.quats << ", min_vec3: " << sampler.min_vec3
			   << ", max_vec3: " << sampler.max_vec3 << ", min_quat: " << sampler.min_quat
			   << "max_quat: " << sampler.max_quat << "}";
			return os;
		}
	};

	struct Gltf_Animation {
		std::vector<Gltf_Channel> channels{};
		std::vector<Gltf_Sampler> samplers{};
		std::string name = "";
	};

	struct Gltf_Skin {
		std::vector<uint32_t> joints;
		std::vector<glm::mat4> inverse_bind_poses{1.f};
	};

	struct Gltf_Model {
		std::vector<Gltf_Mesh> meshes;
		std::vector<Gltf_Node> nodes;
		std::vector<Gltf_Scene> scenes;
		std::vector<Gltf_Material> materials;
		std::vector<std::string> texture_paths;
		std::vector<Gltf_Texture> textures;
		std::vector<Gltf_Animation> animations;
		std::vector<Gltf_Skin> skins;
		int default_scene_index = -1;
		int default_skin_index = -1;
		std::string get_primitive_texture_path(uint32_t mesh_index, uint32_t primitive_index);
		Gltf_Texture get_primitive_texture(uint32_t mesh_index, uint32_t primitive_index);
		glm::vec3 get_bounding_box() const;
	};

	Gltf_Model parse_gltf_model(const std::string& path);

} // namespace Parser