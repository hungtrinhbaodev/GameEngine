#pragma once
#include <array>
#include <functional>
#include <geometry_structs.h>
#include <iostream>
#include <parser/font_parser.h>
#include <parser/gltf_parser.h>
#include <sparse_set.h>
#include <vulkan/vk_buffer.h>
#include <vulkan/vk_consts.h>
#include <vulkan/vk_image.h>

namespace Vulkan {

	struct Draw_Information {
		Const::DRAW_ID draw_type = Const::DRAW_ID::UNDEFINED;
		/**
		 * @Note: unique material need to draw
		 * this object per draw id.
		 */
		uint32_t draw_material_id = -1;
		/**
		 * @Note: with object draw with multi
		 * instance and sub instance can
		 * be use this to distint!
		 */
		uint32_t sub_unique_id = 0;
		/**
		 * @Note: index of object draw.
		 */
		uint32_t draw_index = 0;
		/**
		 * @Note: turn on/off object to draw.
		 */
		bool visible = false;
		/**
		 * @Note: when object draw has same
		 * draw index the order draw will
		 * be sort by create_index
		 */
		uint32_t create_index = 0;
	};

	struct Draw_Instance_Information {
		uint32_t instance_storage_id = 0;
		/**
		 * @Note: using number sub instance
		 * to font object that one call draw
		 * all char.
		 */
		uint32_t number_sub_instance = 1;
		uint32_t instance_material_id = 0;
		inline bool operator<(const Draw_Instance_Information& other) const {
			return instance_material_id < other.instance_material_id;
		}
		inline friend std::ostream& operator<<(std::ostream& os, const Draw_Instance_Information& instance) {
			os << "{Draw_Instance_Information: {instance_storage_id: " << instance.instance_storage_id
			   << " number_sub_instance: " << instance.number_sub_instance
			   << ", instance_material_id: " << instance.instance_material_id << "}";
			return os;
		}
	};

	struct Draw_Attribute {
		uint32_t draw_index = 0;
		bool is_visible = false;
	};

	struct Rectangle_Attributes {
		float width = 0.f;
		float height = 0.f;
		glm::vec2 position = {0.f, 0.f};
		glm::vec2 anchor = {0.f, 0.f};
		glm::vec3 color = {1.f, 1.f, 1.f};
		float rotation = 0;
	};

	struct Texture_2D_Attributes {
		std::string path = "";
		glm::vec2 position{0.f, 0.f};
		glm::vec2 scale = {1.f, 1.f};
		glm::vec2 anchor = {0.f, 0.f};
		Geometry::Texture_Rect_2D texture_rect{0.f, 0.f, 1.f, 1.f};
		float rotation = 0;
	};

	struct Triangle_Attribultes {
		/**
		 * @Note: 3 point of triangle to draw.
		 */
		glm::vec2 first{0.f, 0.f};
		glm::vec2 second{0.f, 0.f};
		glm::vec2 third{0.f, 0.f};
		glm::vec3 color{1.f, 1.f, 1.f};
	};

	struct Font_2D_Attributes {
		std::string path = "";
		std::string text = "";
		int font_size = 1;
		glm::vec2 position{0.f, 0.f};
		glm::vec2 scale = {1.f, 1.f};
		glm::vec2 anchor = {0.f, 0.f};
		glm::vec3 color = {1.f, 1.f, 1.f};
		float rotation = 0;
		/**
		 * @Note: row align type:
		 * 0: center.
		 * 1: left.
		 * 2: right.
		 */
		int align = 0;
		inline bool operator==(const Font_2D_Attributes& other) const {
			return path == other.path && text == other.text && font_size == other.font_size &&
				   position == other.position && scale == other.scale && anchor == other.anchor &&
				   rotation == other.rotation;
		}

		inline bool operator!=(const Font_2D_Attributes& other) const { return !(*this == other); }
	};

	struct Resource_Synchronize_Information {

		std::array<bool, Const::MAX_FRAMES_IN_FLIGHT> blocked{true};

		void release_frame(uint32_t frame_index);

		bool can_remove();
	};

	struct Buffer_Range {

		uint32_t offset = 0;

		uint32_t align = 0;

		uint32_t size = 0;

		uint32_t stride = 1;

		template <typename T> inline uint32_t size_as() const { return size / sizeof(T); }

		template <typename T> inline uint32_t offset_as() const { return (offset + align) / sizeof(T); }

		uint32_t total_size() const;

		bool is_suitable_with(uint32_t size, uint32_t element_stride = 1) const;

		Buffer_Range fit_with(uint32_t size, uint32_t element_stride = 1);

		uint32_t inner_offset() const;

		uint32_t inner_size() const;

		inline friend std::ostream& operator<<(std::ostream& os, const Buffer_Range& range) {
			os << "Buffer_Range: {offset: " << range.offset << " align: " << range.align << ", size: " << range.size
			   << "}";
			return os;
		}
	};

	struct Texture_View {

		std::string file = "";

		Image image;

		float width;

		float height;

		Const::TEXTURE_STORAGE_MODE storage_mode = Const::TEXTURE_STORAGE_MODE::BUCKET;

		int bucket_index;

		int slot_index;

		glm::vec2 tex_scale() const;
	};

	struct Buffer_Range_Compare {
		bool operator()(const Buffer_Range& a, const Buffer_Range& b) const;
	};

	struct Resize_Buffer_Synchronize_Information {
		bool is_resizing = false;
		Buffer old_buffer{};
		Resource_Synchronize_Information sync_info{};
	};

	struct Copied_Data_Synchronize_Information {
		bool is_copying = false;
		VkFence fence = VK_NULL_HANDLE;
	};

	struct Font : public Parser::Font {
		uint32_t texture_id = 0;
		uint32_t rect_storage_id = 0;
		uint32_t rect_storage_offset = 0;
		char to_char_index(char value) const;
		glm::vec2 get_char_bounding_box(char value) const;
		glm::vec2 get_char_local_offset(char value) const;
		float get_char_scale(int font_size) const;
		uint32_t get_char_rect_offset(char value) const;
	};

	struct Model_Node {
		glm::vec3 translation{0.f};
		glm::vec3 scale{1.f};
		glm::quat rotation{1.f, 0.f, 0.f, 0.f};
		glm::mat4 transform{1.f};
		std::vector<int> children_indices{};
		int mesh_index = -1;
		int skin_index = -1;
		bool is_use_matrix = false;
		void parse_from(Parser::Gltf_Node& node);
		glm::mat4 get_transform() const;
	};

	struct Model_Scene {
		std::vector<uint32_t> node_root_indices;
		std::string name = "";
		void parse_from(Parser::Gltf_Scene& scene);
	};

	struct Model_Primitive {
		uint32_t vertex_id = 0;
		uint32_t indices_id = 0;
		uint32_t texture_id = 0;
		inline friend std::ostream& operator<<(std::ostream& os, const Model_Primitive& primitive) {
			os << "Model_Primitive: {vertex_id: " << primitive.vertex_id << ", indices_id: " << primitive.indices_id
			   << ", texture_id: " << primitive.texture_id << "}";
			return os;
		}
	};

	struct Model_Skin {
		std::vector<uint32_t> joints{};
		std::vector<glm::mat4> inverse_bind_poses{1.f};
		void parse_from(Parser::Gltf_Skin& skin);
	};

	enum Model_Sampler_Interpolation_Type {
		LINEAR,
		INTERPOLATION_UNSUPPORTED
	};

	struct Model_Sampler {
		uint32_t number_apply = 0;
		Model_Sampler_Interpolation_Type interpolation = Model_Sampler_Interpolation_Type::INTERPOLATION_UNSUPPORTED;
		std::vector<float> durations{};
		std::vector<glm::vec3> vec3s{};
		std::vector<glm::quat> quats{};
		glm::vec3 min_vec3{0};
		glm::vec3 max_vec3{0};
		glm::quat min_quat{1.f, 0.f, 0.f, 0.f};
		glm::quat max_quat{1.f, 0.f, 0.f, 0.f};
		void parse_from(Parser::Gltf_Sampler& sampler);
		float max_duration() const;
		glm::vec3 sampler_vec3(float t, int& last_index) const;
		glm::quat sampler_quat(float t, int& last_index) const;
	};

	enum Model_Sampler_Type {
		TRANSLATION,
		ROTATION,
		SCALE,
		SAMPLER_UNSUPPORTED
	};

	struct Model_Channel {
		bool use_node_index = false;
		int sampler_index = 0;
		Model_Sampler_Type sampler_type = Model_Sampler_Type::SAMPLER_UNSUPPORTED;
		int node_index = 0;
		void parse_from(Parser::Gltf_Channel& channel);
	};

	struct Model_Animation {
		std::vector<Model_Channel> channels{};
		std::vector<Model_Sampler> samplers{};
		std::string name = "";
		void parse_from(Parser::Gltf_Animation& animation);
	};

	struct Model_Mesh {
		std::vector<int> primitive_indices;
		glm::vec3 min_bounding_box{0.f};
		glm::vec3 max_bounding_box{0.f};
		glm::vec3 get_mesh_origin() const;
	};

	struct Model_Mesh_Draw_Info {
		int mesh_index = -1;
		int skin_index = -1;
		glm::mat4 mesh_transform{1.f};
		inline friend std::ostream& operator<<(std::ostream& os, const Model_Mesh_Draw_Info& info) {
			os << "Model_Mesh_Draw_Info: {mesh_index: " << info.mesh_index << ", skin_index: " << info.skin_index
			   << ", mesh_transform: " << info.mesh_transform << "}";
			return os;
		}
	};

	struct Model {
		std::vector<Model_Node> nodes;
		std::vector<Model_Mesh> meshes;
		std::vector<Model_Skin> skins;
		std::vector<Model_Animation> animations;
		std::vector<Model_Scene> scenes;
		std::vector<Model_Primitive> primitives;
		Sparse_Set<std::vector<Model_Mesh_Draw_Info>> scene_index_to_draw_meshes_info{};
		int default_scene_index = -1;
		int default_skin_index = -1;
		int find_scene_index_by(const std::string& name) const;
		int find_animation_index_by(const std::string& name) const;
		void dfs_write(int scene_index, std::function<void(Model&, int, int, int)> execute);
		void dfs_read(int scene_index, std::function<void(const Model&, int, int, int)> execute) const;
	};

	struct Draw_2D_Common_Data {
		glm::vec2 screen_size = {0.f, 0.f};
	};
} // namespace Vulkan