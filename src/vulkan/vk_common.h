#pragma once
#include <array>
#include <geometry_structs.h>
#include <iostream>
#include <parser/font_parser.h>
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

	struct Model_3D_Attributes {
		std::string path = "";
		glm::vec3 position = {0.f, 0.f, 0.f};
		glm::vec3 scale = {1.f, 1.f, 1.f};
		glm::vec3 rotation = {0.f, 0.f, 0.f};
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

		friend std::ostream& operator<<(std::ostream& os, const Buffer_Range& range) {
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

	struct Primitive_Buffer_Range {
		uint32_t flat_id = 0;
		uint32_t vertex_id = 0;
		uint32_t indices_id = 0;
		uint32_t texture_id = 0;
	};

	struct Model_Mesh_Information {
		std::vector<Primitive_Buffer_Range> primitives;
		glm::vec3 min_bounding_box{0.f};
		glm::vec3 max_bounding_box{0.f};
		std::unordered_map<uint32_t, uint32_t> mesh_transforms_storage_id{};
		std::unordered_map<uint32_t, std::vector<uint32_t>> mesh_transform_storage_offsets{};
		const std::vector<uint32_t>& get_mesh_transform_offsets(uint32_t scene_index) const;
		glm::vec3 get_mesh_origin() const;
	};

	struct Model_Information {
		std::vector<Model_Mesh_Information> meshes;
		std::unordered_map<uint32_t, std::unordered_map<uint32_t, std::vector<glm::mat4>>> global_meshes_transformes;
		std::vector<Primitive_Buffer_Range> flat_primitives{};
		uint32_t draw_scene_index = -1;
		uint32_t get_number_primitive() const;
		uint32_t get_primitive_flat_index(const uint32_t& mesh_index, const uint32_t& primitive_index) const;
	};

	struct Draw_2D_Common_Data {
		glm::vec2 screen_size = {0.f, 0.f};
	};
} // namespace Vulkan