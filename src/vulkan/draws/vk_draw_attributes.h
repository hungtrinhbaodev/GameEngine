#pragma once
#include <string>

#include <geometry_structs.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <handle.h>
#include <vulkan/vk_consts.h>
#include <vulkan/vulkan.h>

namespace Vulkan {

	struct Draw_Attributes_Description {
		uint32_t draw_index = 0;
		bool is_visible = true;
	};

	class Draw_Atributes_2 {
	  private:
		Handle handle;
		uint32_t draw_index = 0;
		bool visible = true;

	  protected:
		void assert_handle() const;
		void set_dirty(bool dirty) const;

	  public:
		Draw_Atributes_2();
		Draw_Atributes_2(Handle handle, Draw_Attributes_Description description);
		~Draw_Atributes_2();
		void set_visible(bool visible);
		void set_draw_index(uint32_t draw_index);

		bool is_visible() const;
		uint32_t get_draw_index() const;
		Draw_Attributes_Description get_description() const;
	};

	struct Model_3D_Description : public Draw_Attributes_Description {
		std::string path = "";
		std::string scene_name = "";
		std::string animation_name = "";
		glm::vec3 position{0.f}, scale{1.f}, rotation{0.f};
	};

	class Model_3D : public Draw_Atributes_2 {
	  private:
		std::string path = "";
		glm::vec3 position = {0.f, 0.f, 0.f};
		glm::vec3 scale = {1.f, 1.f, 1.f};
		glm::vec3 rotation{0.f, 0.f, 0.f};
		glm::quat quat_rotation{1.f, 0.f, 0.f, 0.f};
		std::string scene_name = "";
		std::string animation_name = "";

	  public:
		Model_3D();
		Model_3D(Handle handle, Model_3D_Description description);
		~Model_3D();

		void set_model_path(const std::string& model_path);
		void set_position(const glm::vec3& position);
		void set_scale(const glm::vec3& scale);
		/**
		 * @Note: rotation will be store as degree.
		 */
		void set_rotation(const glm::vec3& rotation);
		void set_scene_name(const std::string& scene_name);
		void set_animation_name(const std::string& animation_name);

		std::string get_model_path() const;
		glm::vec3 get_position() const;
		glm::vec3 get_scale() const;
		/**
		 * @Note: rotation will be store as degree.
		 */
		glm::vec3 get_rotation() const;
		glm::quat get_quat_rotation() const;
		std::string get_scene_name() const;
		std::string get_animation_name() const;
		glm::mat4 get_transform() const;
		Model_3D_Description get_description() const;
	};

	struct Draw_2D_Attributes_Description : public Draw_Attributes_Description {
		glm::vec2 position{0.f};
		glm::vec2 scale{1.f};
		/**
		 * @Note: rotation in degree.
		 */
		float rotation = 0.f;
		glm::vec2 anchor{0.f};
		glm::vec3 color{1.f};
	};

	class Draw_2D : public Draw_Atributes_2 {
	  private:
		glm::vec2 position{0.f};
		glm::vec2 scale{1.f};
		float rotation = 0.f;
		glm::vec2 anchor{0.f};
		glm::vec3 color{1.f};

	  public:
		Draw_2D();
		Draw_2D(Handle handle, Draw_2D_Attributes_Description description);
		~Draw_2D();

		void set_position(const glm::vec2& position);
		/**
		 * @Note: rotation in degree.
		 */
		void set_rotation(float rotation);
		void set_scale(const glm::vec2& scale);
		void set_anchor(const glm::vec2& anchor);
		void set_color(const glm::vec3& color);

		glm::vec2 get_position() const;
		float get_rotation() const;
		glm::vec2 get_scale() const;
		glm::vec2 get_anchor() const;
		glm::vec3 get_color() const;
		Draw_2D_Attributes_Description get_description() const;
	};

	struct Label_2D_Description : public Draw_2D_Attributes_Description {
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
	};

	class Label_2D : public Draw_2D {
	  private:
		std::string path = "";
		std::string text = "";
		int font_size = 12;
		int align = 0;
		float width = 0.f;
		float height = 0.f;
		bool text_dirty = false;
		void update_text_size();

	  public:
		Label_2D();
		Label_2D(Handle handle, Label_2D_Description description);
		~Label_2D();

		void set_font_path(const std::string& path);
		void set_text(const std::string& text);
		void set_font_size(int font_size);
		void set_align(int align);
		void set_width(float width);
		void set_height(float height);

		float get_width();
		float get_height();
		std::string get_font_path() const;
		std::string get_text() const;
		int get_font_size() const;
		int get_align() const;
		Label_2D_Description get_description() const;
	};

	struct Texture_2D_Description : public Draw_2D_Attributes_Description {
		std::string path = "";
		Geometry::Texture_Rect_2D tex_rect{0.f, 0.f, 1.f, 1.f};
	};

	class Texture_2D : public Draw_2D {
	  private:
		std::string path = "";
		Geometry::Texture_Rect_2D tex_rect = {0.f, 0.f, 1.f, 1.f};

	  public:
		Texture_2D();
		Texture_2D(Handle handle, Texture_2D_Description description);
		~Texture_2D();

		void set_texture_path(const std::string& path);
		void set_texture_rect(const Geometry::Texture_Rect_2D& tex_rect);

		std::string get_texture_path() const;
		Geometry::Texture_Rect_2D get_texture_rect() const;
		Texture_2D_Description get_description() const;
	};

}; // namespace Vulkan