#pragma once
#include <string>

#include <array>
#include <consts.h>
#include <geometry_structs.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <handle.h>
#include <vulkan/vk_consts.h>

namespace Vulkan {

	struct Clipping_Helper {
		uint32_t storage_offset = ::Const::MAX_UINT32;
	};

	struct Font_Helper {
		std::vector<std::string> lines{};
		std::vector<glm::vec2> start_line_positions{};
		float width = 0;
		float height = 0;
		glm::vec2 get_size();
	};

	struct Texture_Helper {
		float width = 0.f;
		float height = 0.f;
		glm::vec2 get_size();
	};

	struct Model_Animation_Helper {
		std::string animation_name = "";
		float duration = 0.f;
	};

	struct Draw_Attributes_Description {
		uint32_t draw_index = 0;
		bool is_visible = true;
		float opacity = 1.f;
		glm::vec3 color{1.f, 1.f, 1.f};
	};

	struct Clipping_Ancestor_Storage {
		std::array<uint32_t, ::Const::MAX_DEPTH_CLIPPING> clipping_offsets;
		uint32_t number_clipping = 0;
	};

	class Base_Attributes {
	  private:
		Handle handle;

	  protected:
		void assert_handle() const;
		void set_dirty(bool dirty) const;

	  public:
		Base_Attributes();
		Base_Attributes(const Handle& handle);
		~Base_Attributes();
	};

	class Draw_Atributes : public Base_Attributes {
	  private:
		uint32_t draw_index = 0;
		bool visible = true;
		float opacity = 1.f;
		glm::vec3 color{1.f, 1.f, 1.f};
		uint32_t number_clipping = 0;
		std::array<Handle, ::Const::MAX_DEPTH_CLIPPING> clipping_ancestors{};

	  public:
		Draw_Atributes();
		Draw_Atributes(const Handle& handle, const Draw_Attributes_Description& description);
		~Draw_Atributes();
		void set_visible(bool visible);
		void set_draw_index(uint32_t draw_index);
		void set_opacity(float opacity);
		void set_color(const glm::vec3& color);
		void set_number_clipping(uint32_t number_clipping);
		void set_clipping_ancestors(const std::array<Handle, ::Const::MAX_DEPTH_CLIPPING>& clipping_ancestors);

		bool is_visible() const;
		uint32_t get_draw_index() const;
		float get_opacity() const;
		glm::vec3 get_color() const;
		uint32_t get_number_clipping() const;
		const std::array<Handle, ::Const::MAX_DEPTH_CLIPPING>& get_clipping_ancestors() const;
		Draw_Attributes_Description get_description() const;
	};

	struct Model_3D_Animation_Description {
		bool is_active = false;
		std::string animation_name = "";
		bool is_loop = false;
		float factor = 0.f;
		float speed = 1.f;
	};

	struct Model_3D_Description : public Draw_Attributes_Description {
		std::string path = "";
		std::string scene_name = "";
		glm::vec3 position{0.f}, scale{1.f}, rotation{0.f};
		float blend_animation_duration = 0.2f;
		float blend_animation_factor = 1.f;
		std::vector<std::string> animations_component_paths;
	};

	class Model_3D : public Draw_Atributes {
	  private:
		std::string path = "";
		glm::vec3 position = {0.f, 0.f, 0.f};
		glm::vec3 scale = {1.f, 1.f, 1.f};
		glm::vec3 rotation{0.f, 0.f, 0.f};
		glm::quat quat_rotation{1.f, 0.f, 0.f, 0.f};
		std::string scene_name = "";
		float blend_animation_duration = 0.f;
		float blend_animation_factor = 0.f;
		std::array<bool, Const::MAX_TRACK_MODEL_ANIMATION> changing_animations;
		std::array<Model_3D_Animation_Description, Const::MAX_TRACK_MODEL_ANIMATION> animations;
		std::array<float, Const::MAX_TRACK_MODEL_ANIMATION> animation_factors;
		std::array<double, Const::MAX_TRACK_MODEL_ANIMATION> animation_start_times;
		std::vector<std::string> animations_component_paths;

		void reload_animation_factors();
		void clean_animations();

	  public:
		Model_3D();
		Model_3D(const Handle& handle, const Model_3D_Description& description);
		~Model_3D();

		void update_data_before_modify();
		void set_model_path(const std::string& model_path);
		void set_position(const glm::vec3& position);
		void set_scale(const glm::vec3& scale);
		/**
		 * @Note: rotation will be store as degree.
		 */
		void set_rotation(const glm::vec3& rotation);
		void set_scene_name(const std::string& scene_name);
		void set_blend_animation_duration(float blend_duration);
		void set_blend_animation_factor(float blend_factor);
		/**
		 * @Note: animations will be supported in 3 track
		 * each track factor and speed will be applied with
		 * param factor and speed, which speed large animation
		 * will be run fast!
		 */
		void play_animation(
			int track_index, const std::string& animation_name, bool is_loop = false, float factor = 1.f,
			float speed = 1.f
		);

		std::string get_model_path() const;
		glm::vec3 get_position() const;
		glm::vec3 get_scale() const;
		/**
		 * @Note: rotation will be store as degree.
		 */
		glm::vec3 get_rotation() const;
		glm::quat get_quat_rotation() const;
		std::string get_scene_name() const;
		glm::mat4 get_transform() const;
		Model_3D_Description get_description() const;
		Model_3D_Animation_Description view_animation_at(int track_index) const;
		const std::vector<std::string>& get_animations_component_paths() const;
		std::vector<std::string> view_all_animation_name() const;
		float get_blend_animation_duration() const;
		float get_blend_animation_factor() const;

		const std::array<Model_3D_Animation_Description, Const::MAX_TRACK_MODEL_ANIMATION>& get_animations() const;
		bool is_track_changing(int track_index) const;
		std::array<float, Const::MAX_TRACK_MODEL_ANIMATION> get_animation_factors() const;
		void clean_changing_animations();
	};

	struct Draw_2D_Attributes_Description : public Draw_Attributes_Description {
		glm::vec2 position{0.f};
		/**
		 * @Note: rotation in degree.
		 */
		float rotation = 0.f;
		glm::vec2 anchor{0.f};
	};

	class Draw_2D : public Draw_Atributes {
	  private:
		glm::vec2 position{0.f};
		float rotation = 0.f;
		glm::vec2 anchor{0.f};

	  public:
		Draw_2D();
		Draw_2D(const Handle& handle, const Draw_2D_Attributes_Description& description);
		~Draw_2D();

		void set_position(const glm::vec2& position);
		/**
		 * @Note: rotation in degree.
		 */
		void set_rotation(float rotation);
		void set_anchor(const glm::vec2& anchor);

		glm::vec2 get_position() const;
		float get_rotation() const;
		glm::vec2 get_anchor() const;
		Draw_2D_Attributes_Description get_description() const;
	};

	struct Draw_2D_Scale_Able_Description : public Draw_2D_Attributes_Description {
		glm::vec2 scale{1.f};
	};

	class Draw_2D_Scale_Aable : public Draw_2D {
	  private:
		glm::vec2 scale{1.f};

	  public:
		Draw_2D_Scale_Aable();
		Draw_2D_Scale_Aable(const Handle& handle, const Draw_2D_Scale_Able_Description& descripton);
		~Draw_2D_Scale_Aable();

		void set_scale(const glm::vec2& scale);

		glm::vec2 get_scale() const;
		Draw_2D_Scale_Able_Description get_description() const;
	};

	struct Label_2D_Description : public Draw_2D_Scale_Able_Description {
		std::string path = "";
		std::string text = "";
		int font_size = 12;
		/**
		 * @Note: row align type:
		 * 0: center.
		 * 1: left.
		 * 2: right.
		 */
		int align = 0;
		float stroke_size = 0.f;
		glm::vec3 stroke_color = {0.f, 0.f, 0.f};
	};

	class Font_2D : public Draw_2D_Scale_Aable {
	  private:
		std::string path = "";
		std::string text = "";
		int font_size = 12;
		int align = 0;
		float width = 0.f;
		float height = 0.f;
		float stroke_size = 0.f;
		glm::vec3 stroke_color = {0.f, 0.f, 0.f};
		bool text_dirty = false;
		void reload_text_size();

	  public:
		Font_2D();
		Font_2D(const Handle& handle, const Label_2D_Description& description);
		~Font_2D();

		void set_font_path(const std::string& path);
		void set_text(const std::string& text);
		void set_font_size(int font_size);
		void set_align(int align);
		void set_width(float width);
		void set_height(float height);
		void set_stroke_size(float size);
		void set_stroke_color(const glm::vec3& color);

		float get_width();
		float get_height();
		std::string get_font_path() const;
		std::string get_text() const;
		int get_font_size() const;
		int get_align() const;
		float get_stroke_size() const;
		glm::vec3 get_stroke_color() const;
		Label_2D_Description get_description() const;
	};

	struct Texture_2D_Description : public Draw_2D_Scale_Able_Description {
		std::string path = "";
		Geometry::Texture_Rect_2D tex_rect{0.f, 0.f, 1.f, 1.f};
	};

	class Texture_2D : public Draw_2D_Scale_Aable {
	  private:
		std::string path = "";
		Geometry::Texture_Rect_2D tex_rect = {0.f, 0.f, 1.f, 1.f};
		float width = 0.f;
		float height = 0.f;
		bool texture_dirty = false;
		void update_texture_size();

	  public:
		Texture_2D();
		Texture_2D(const Handle& handle, const Texture_2D_Description& description);
		~Texture_2D();

		void set_texture_path(const std::string& path);
		void set_texture_rect(const Geometry::Texture_Rect_2D& tex_rect);

		float get_width();
		float get_height();
		std::string get_texture_path() const;
		Geometry::Texture_Rect_2D get_texture_rect() const;
		Texture_2D_Description get_description() const;
	};

	struct Rectangle_2D_Description : Draw_2D_Attributes_Description {
		float width = 0;
		float height = 0;
		float border_radius = 0.f;
		float border_width = 0.f;
		glm::vec3 border_color = {0.f, 0.f, 0.f};
	};

	class Rectangle_2D : public Draw_2D {
	  private:
		float width = 0;
		float height = 0;
		float border_radius = 0.f;
		float border_width = 0.f;
		glm::vec3 border_color = {0.f, 0.f, 0.f};

	  public:
		Rectangle_2D();
		Rectangle_2D(const Handle& handle, const Rectangle_2D_Description& description);
		~Rectangle_2D();

		void set_width(const float& width);
		void set_height(const float& height);
		void set_border_radius(const float& border_radius);
		void set_border_width(const float& border_width);
		void set_border_color(const glm::vec3& color);

		float get_width() const;
		float get_height() const;
		float get_border_radius() const;
		float get_border_width() const;
		glm::vec3 get_border_color() const;
		Rectangle_2D_Description get_description() const;
	};

	struct Triangle_2D_Description : public Draw_Attributes_Description {
		glm::vec2 first{0.f, 0.f};
		glm::vec2 second{0.f, 0.f};
		glm::vec2 third{0.f, 0.f};
	};

	class Triangle_2D : public Draw_Atributes {
	  private:
		glm::vec2 first{0.f, 0.f};
		glm::vec2 second{0.f, 0.f};
		glm::vec2 third{0.f, 0.f};

	  public:
		Triangle_2D();
		Triangle_2D(const Handle& handle, const Triangle_2D_Description& description);
		~Triangle_2D();

		void set_first_point(const glm::vec2& first);
		void set_second_point(const glm::vec2& second);
		void set_third_point(const glm::vec2& third);

		glm::vec2 get_first_point() const;
		glm::vec2 get_second_point() const;
		glm::vec2 get_third_point() const;
		Triangle_2D_Description get_description() const;
	};

	struct Clipping_Description {
		glm::vec2 size{0.f, 0.f};
		float border_size = 0.f;
		float border_width = 0.f;
		glm::vec2 world_position{0.f, 0.f};
		glm::vec2 world_scale{0.f, 0.f};
		glm::vec2 anchor{0.f, 0.f};
		float world_rotation = 0.f;
	};

	class Clipping_2D : public Base_Attributes {
	  private:
		Handle handle;
		glm::vec2 size{0.f, 0.f};
		float border_size = 0.f;
		float border_width = 0.f;
		glm::vec2 world_position{0.f, 0.f};
		glm::vec2 world_scale{0.f, 0.f};
		glm::vec2 anchor{0.f, 0.f};
		float world_rotation = 0.f;

	  public:
		Clipping_2D();
		Clipping_2D(const Handle& handle, const Clipping_Description& description);
		~Clipping_2D();

		void set_content_size(const glm::vec2& size);
		void set_border_radius(float border_radius);
		void set_world_position(const glm::vec2& world_position);
		void set_world_scale(const glm::vec2& world_scale);
		void set_world_rotation(float rotation);
		void set_anchor(const glm::vec2& anchor);
		void set_border_width(float border_width);

		glm::vec2 get_content_size() const;
		float get_border_radius() const;
		glm::vec2 get_world_position() const;
		glm::vec2 get_world_scale() const;
		float get_world_rotation() const;
		glm::vec2 get_anchor() const;
		float get_border_width() const;
		Clipping_Description get_description() const;
	};

}; // namespace Vulkan