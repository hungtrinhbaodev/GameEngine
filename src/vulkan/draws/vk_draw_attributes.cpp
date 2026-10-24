#include <math_custom.h>
#include <vulkan/draws/vk_draw_attributes.h>
#include <vulkan/vk_draw.h>

namespace Vulkan {
	Draw_Atributes_2::Draw_Atributes_2() {}

	Draw_Atributes_2::Draw_Atributes_2(Handle i_handle, Draw_Attributes_Description description) {
		handle = i_handle;
		draw_index = description.draw_index;
		visible = description.is_visible;
	}

	Draw_Atributes_2::~Draw_Atributes_2() {}

	void Draw_Atributes_2::assert_handle() const {
		assert_draw_handle(handle);
	}

	void Draw_Atributes_2::set_dirty(bool dirty) const {
		if (dirty) {
			set_draw_dirty(handle);
		}
	}

	void Draw_Atributes_2::set_visible(bool i_visible) {
		assert_handle();
		set_dirty(visible != i_visible);
		visible = i_visible;
	}

	void Draw_Atributes_2::set_draw_index(uint32_t i_draw_index) {
		assert_handle();
		set_dirty(draw_index != i_draw_index);
		draw_index = i_draw_index;
	}

	bool Draw_Atributes_2::is_visible() const {
		assert_handle();
		return visible;
	}

	uint32_t Draw_Atributes_2::get_draw_index() const {
		assert_handle();
		return draw_index;
	}

	Draw_Attributes_Description Draw_Atributes_2::get_description() const {
		return {draw_index, visible};
	}

	Model_3D::Model_3D() {}

	Model_3D::Model_3D(Handle i_handle, Model_3D_Description description)
		: Draw_Atributes_2(i_handle, description) {
		path = description.path;
		position = description.position;
		scale = description.scale;
		rotation = description.rotation;
		quat_rotation = glm::quat(glm::radians(description.rotation));
		scene_name = description.scene_name;
		animation_name = description.animation_name;
	}

	Model_3D::~Model_3D() {}

	void Model_3D::set_model_path(const std::string& i_model_path) {
		assert_handle();
		set_dirty(path != i_model_path);
		path = i_model_path;
	}

	void Model_3D::set_position(const glm::vec3& i_position) {
		assert_handle();
		set_dirty(position != i_position);
		position = i_position;
	}

	void Model_3D::set_scale(const glm::vec3& i_scale) {
		assert_handle();
		set_dirty(scale != i_scale);
		scale = i_scale;
	}

	void Model_3D::set_rotation(const glm::vec3& i_rotation) {
		assert_handle();
		set_dirty(rotation != i_rotation);
		rotation = i_rotation;
		quat_rotation = glm::quat(glm::radians(i_rotation));
	}

	void Model_3D::set_scene_name(const std::string& i_scene_name) {
		assert_handle();
		set_dirty(scene_name != i_scene_name);
		scene_name = i_scene_name;
	}

	void Model_3D::set_animation_name(const std::string& i_animation_name) {
		assert_handle();
		set_dirty(animation_name != i_animation_name);
		animation_name = i_animation_name;
	}

	std::string Model_3D::get_model_path() const {
		assert_handle();
		return path;
	}

	glm::vec3 Model_3D::get_position() const {
		assert_handle();
		return position;
	}

	glm::vec3 Model_3D::get_scale() const {
		assert_handle();
		return scale;
	}

	glm::vec3 Model_3D::get_rotation() const {
		assert_handle();
		return rotation;
	}

	glm::quat Model_3D::get_quat_rotation() const {
		assert_handle();
		return quat_rotation;
	}

	std::string Model_3D::get_scene_name() const {
		assert_handle();
		return scene_name;
	}

	std::string Model_3D::get_animation_name() const {
		assert_handle();
		return animation_name;
	}

	glm::mat4 Model_3D::get_transform() const {
		assert_handle();
		return Math::make_translation(position) * glm::mat4_cast(quat_rotation) * Math::make_scale(scale);
	}

	Model_3D_Description Model_3D::get_description() const {
		return Model_3D_Description{
			Draw_Atributes_2::get_description(), path, scene_name, animation_name, position, scale, rotation
		};
	}

	Draw_2D::Draw_2D() {}

	Draw_2D::Draw_2D(Handle handle, Draw_2D_Attributes_Description description)
		: Draw_Atributes_2(handle, description) {
		position = description.position;
		rotation = description.rotation;
		scale = description.scale;
		anchor = description.anchor;
		color = description.color;
	}

	Draw_2D::~Draw_2D() {}

	void Draw_2D::set_position(const glm::vec2& i_position) {
		assert_handle();
		set_dirty(position != i_position);
		position = i_position;
	}

	void Draw_2D::set_rotation(float i_rotation) {
		assert_handle();
		set_dirty(rotation != i_rotation);
		rotation = i_rotation;
	}

	void Draw_2D::set_scale(const glm::vec2& i_scale) {
		assert_handle();
		set_dirty(scale != i_scale);
		scale = i_scale;
	}

	void Draw_2D::set_anchor(const glm::vec2& i_anchor) {
		assert_handle();
		set_dirty(anchor != i_anchor);
		anchor = i_anchor;
	}

	void Draw_2D::set_color(const glm::vec3& i_color) {
		assert_handle();
		set_dirty(color != i_color);
		color = i_color;
	}

	glm::vec2 Draw_2D::get_position() const {
		assert_handle();
		return position;
	}

	float Draw_2D::get_rotation() const {
		assert_handle();
		return rotation;
	}

	glm::vec2 Draw_2D::get_scale() const {
		assert_handle();
		return scale;
	}

	glm::vec2 Draw_2D::get_anchor() const {
		assert_handle();
		return anchor;
	}

	glm::vec3 Draw_2D::get_color() const {
		assert_handle();
		return color;
	}

	Draw_2D_Attributes_Description Draw_2D::get_description() const {
		Draw_2D_Attributes_Description description{};
		static_cast<Draw_Attributes_Description&>(description) = Draw_Atributes_2::get_description();
		description.position = position;
		description.scale = scale;
		description.rotation = rotation;
		description.anchor = anchor;
		description.color = color;
		return description;
	}

	Label_2D::Label_2D() {}

	Label_2D::Label_2D(Handle handle, Label_Description description)
		: Draw_2D(handle, description) {
		path = description.path;
		text = description.text;
		font_size = description.font_size;
		align = description.align;
	}

	Label_2D::~Label_2D() {}

	void Label_2D::set_font_path(const std::string& i_path) {
		assert_handle();
		set_dirty(path != i_path);
		path = i_path;
	}

	void Label_2D::set_text(const std::string& i_text) {
		assert_handle();
		set_dirty(text != i_text);
		text = i_text;
	}

	void Label_2D::set_font_size(int i_font_size) {
		assert_handle();
		set_dirty(font_size != i_font_size);
		font_size = i_font_size;
	}

	void Label_2D::set_align(int i_align) {
		assert_handle();
		set_dirty(align != i_align);
		align = i_align;
	}

	std::string Label_2D::get_font_path() const {
		assert_handle();
		return path;
	}

	std::string Label_2D::get_text() const {
		assert_handle();
		return text;
	}

	int Label_2D::get_font_size() const {
		assert_handle();
		return font_size;
	}

	int Label_2D::get_align() const {
		assert_handle();
		return align;
	}

	Label_Description Label_2D::get_description() const {
		Label_Description description{};
		static_cast<Draw_2D_Attributes_Description&>(description) = Draw_2D::get_description();
		description.path = path;
		description.text = text;
		description.font_size = font_size;
		description.align = align;
		return description;
	}

} // namespace Vulkan
