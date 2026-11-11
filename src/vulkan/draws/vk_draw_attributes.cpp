#include <math_custom.h>
#include <vulkan/draws/vk_draw_attributes.h>
#include <vulkan/draws/vk_draw_font_2D.h>
#include <vulkan/draws/vk_draw_model_3D.h>
#include <vulkan/draws/vk_draw_texture_2D.h>
#include <vulkan/vk_draw.h>

namespace Vulkan {

	glm::vec2 Font_Helper::get_size() {
		return {width, height};
	}

	Base_Attributes::Base_Attributes() {}

	Base_Attributes::Base_Attributes(const Handle& i_handle) {
		handle = i_handle;
	}

	Base_Attributes::~Base_Attributes() {}

	void Base_Attributes::assert_handle() const {
		assert_draw_handle(handle);
	}

	void Base_Attributes::set_dirty(bool dirty) const {
		if (dirty) {
			set_draw_dirty(handle);
		}
	}

	Draw_Atributes::Draw_Atributes() {}

	Draw_Atributes::Draw_Atributes(const Handle& handle, const Draw_Attributes_Description& description)
		: Base_Attributes(handle) {
		draw_index = description.draw_index;
		visible = description.is_visible;
		opacity = std::max(std::min(description.opacity, 1.f), 0.f);
		color = description.color;
	}

	Draw_Atributes::~Draw_Atributes() {}

	void Draw_Atributes::set_visible(bool i_visible) {
		assert_handle();
		set_dirty(visible != i_visible);
		visible = i_visible;
	}

	void Draw_Atributes::set_draw_index(uint32_t i_draw_index) {
		assert_handle();
		set_dirty(draw_index != i_draw_index);
		draw_index = i_draw_index;
	}

	void Draw_Atributes::set_opacity(float i_opacity) {
		assert_handle();
		i_opacity = std::max(std::min(i_opacity, 1.f), 0.f);
		set_dirty(opacity != i_opacity);
		opacity = i_opacity;
	}

	void Draw_Atributes::set_color(const glm::vec3& i_color) {
		assert_handle();
		set_dirty(color != i_color);
		color = i_color;
	}

	void Draw_Atributes::set_number_clipping(uint32_t i_number_clipping) {
		assert_handle();
		set_dirty(number_clipping != i_number_clipping);
		number_clipping = i_number_clipping;
	}

	void Draw_Atributes::set_clipping_ancestors(
		const std::array<Handle, ::Const::MAX_DEPTH_CLIPPING>& i_clipping_ancestors
	) {
		assert_handle();
		bool is_dirty = false;
		for (int i = 0; i < ::Const::MAX_DEPTH_CLIPPING; i++) {
			if (clipping_ancestors[i].id() != i_clipping_ancestors[i].id()) {
				is_dirty = true;
				break;
			}
		}
		set_dirty(is_dirty);
		clipping_ancestors = i_clipping_ancestors;
	}

	bool Draw_Atributes::is_visible() const {
		assert_handle();
		return visible;
	}

	uint32_t Draw_Atributes::get_draw_index() const {
		assert_handle();
		return draw_index;
	}

	float Draw_Atributes::get_opacity() const {
		assert_handle();
		return opacity;
	}

	glm::vec3 Draw_Atributes::get_color() const {
		assert_handle();
		return color;
	}

	uint32_t Draw_Atributes::get_number_clipping() const {
		assert_handle();
		return number_clipping;
	}

	const std::array<Handle, ::Const::MAX_DEPTH_CLIPPING>& Draw_Atributes::get_clipping_ancestors() const {
		assert_handle();
		return clipping_ancestors;
	}

	Draw_Attributes_Description Draw_Atributes::get_description() const {
		return {draw_index, visible, opacity, color};
	}

	Model_3D::Model_3D() {}

	Model_3D::Model_3D(const Handle& handle, const Model_3D_Description& description)
		: Draw_Atributes(handle, description) {
		path = description.path;
		position = description.position;
		scale = description.scale;
		rotation = description.rotation;
		quat_rotation = glm::quat(glm::radians(description.rotation));
		scene_name = description.scene_name;
		blend_animation_duration = description.blend_animation_duration;
		blend_animation_factor = std::min(std::max(description.blend_animation_factor, 0.f), 1.f);
		animations_component_paths = description.animations_component_paths;
	}

	Model_3D::~Model_3D() {}

	void Model_3D::reload_animation_factors() {
		float sum_factor = 0;
		for (int i = 0; i < Const::MAX_TRACK_MODEL_ANIMATION; i++) {
			const auto& animation = animations[i];
			if (animation.is_active) {
				sum_factor += animations[i].factor;
			}
		}
		for (int i = 0; i < Const::MAX_TRACK_MODEL_ANIMATION; i++) {
			const auto& animation = animations[i];
			if (animation.is_active && sum_factor > 0) {
				animation_factors[i] = animations[i].factor / sum_factor;
			} else {
				animation_factors[i] = 0.f;
			}
		}
	}

	void Model_3D::clean_animations() {
		for (int i = 0; i < Const::MAX_TRACK_MODEL_ANIMATION; i++) {
			animations[i] = {};
			animation_factors[i] = 0.f;
			animation_start_times[i] = Utils::now();
		}
	}

	void Model_3D::update_data_before_modify() {
		const auto& model_helper = Draw_Model_3D::get_model_helper(path, animations_component_paths);
		for (int i = 0; i < Const::MAX_TRACK_MODEL_ANIMATION; i++) {
			auto& animation = animations[i];
			if (animation.is_active && model_helper.has_animation(animation.animation_name)) {
				const auto& animation_helper = model_helper.get_animation_heler(animation.animation_name);
				double start_time = animation_start_times[i];
				if ((::Utils::now() - start_time) * animation.speed >= animation_helper.duration &&
					!animation.is_loop) {
					animation.is_active = false;
				}
			}
		}
	}

	void Model_3D::set_model_path(const std::string& i_model_path) {
		assert_handle();
		set_dirty(path != i_model_path);
		if (path != i_model_path) {
			clean_animations();
		}
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

	void Model_3D::set_blend_animation_duration(float blend_duration) {
		assert_handle();
		set_dirty(blend_animation_duration != blend_duration);
		blend_animation_duration = blend_duration;
	}

	void Model_3D::set_blend_animation_factor(float blend_factor) {
		assert_handle();
		blend_factor = std::min(std::max(blend_factor, 0.f), 1.f);
		set_dirty(blend_animation_factor != blend_factor);
		blend_animation_factor = blend_factor;
	}

	void Model_3D::play_animation(
		int track_index, const std::string& animation_name, bool is_loop, float factor, float speed
	) {
		if (track_index < 0 || track_index >= Const::MAX_TRACK_MODEL_ANIMATION)
			return;
		const auto& model_helper = Draw_Model_3D::get_model_helper(path, animations_component_paths);
		auto& animation = animations[track_index];
		if (!model_helper.has_animation(animation_name)) {
			animation = {};
		} else {
			animation.animation_name = animation_name;
			animation.is_loop = is_loop;
			animation.factor = factor;
			animation.is_active = true;
			animation.speed = std::min(std::max(speed, 0.01f), 5.f);
		}
		animation_start_times[track_index] = ::Utils::now();
		changing_animations[track_index] = true;
		reload_animation_factors();
		set_dirty(true);
	}

	bool Model_3D::is_track_changing(int track_index) const {
		if (track_index < 0 || track_index >= Const::MAX_TRACK_MODEL_ANIMATION)
			return false;
		return changing_animations[track_index];
	}

	std::array<float, Const::MAX_TRACK_MODEL_ANIMATION> Model_3D::get_animation_factors() const {
		return animation_factors;
	}

	void Model_3D::clean_changing_animations() {
		for (int i = 0; i < Const::MAX_TRACK_MODEL_ANIMATION; i++) {
			changing_animations[i] = false;
		}
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

	glm::mat4 Model_3D::get_transform() const {
		assert_handle();
		return Math::make_translation(position) * glm::mat4_cast(quat_rotation) * Math::make_scale(scale);
	}

	const std::array<Model_3D_Animation_Description, Const::MAX_TRACK_MODEL_ANIMATION>& Model_3D::
		get_animations() const {
		assert_handle();
		return animations;
	}

	Model_3D_Description Model_3D::get_description() const {
		return Model_3D_Description{
			Draw_Atributes::get_description(),
			path,
			scene_name,
			position,
			scale,
			rotation,
			blend_animation_duration,
			blend_animation_factor,
			animations_component_paths
		};
	}

	Model_3D_Animation_Description Model_3D::view_animation_at(int track_index) const {
		assert_handle();
		if (track_index < 0 || track_index >= Const::MAX_TRACK_MODEL_ANIMATION)
			return {};
		return animations[track_index];
	}

	const std::vector<std::string>& Model_3D::get_animations_component_paths() const {
		assert_handle();
		return animations_component_paths;
	}

	std::vector<std::string> Model_3D::view_all_animation_name() const {
		assert_handle();
		const auto& model_helper = Draw_Model_3D::get_model_helper(path, animations_component_paths);
		std::vector<std::string> animation_names;
		for (const auto& animation : model_helper.animations) {
			animation_names.push_back(animation.animation_name);
		}
		return animation_names;
	}

	float Model_3D::get_blend_animation_duration() const {
		assert_handle();
		return blend_animation_duration;
	}

	float Model_3D::get_blend_animation_factor() const {
		assert_handle();
		return blend_animation_factor;
	}

	Draw_2D::Draw_2D() {}

	Draw_2D::Draw_2D(const Handle& handle, const Draw_2D_Attributes_Description& description)
		: Draw_Atributes(handle, description) {
		position = description.position;
		rotation = description.rotation;
		anchor = description.anchor;
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

	void Draw_2D::set_anchor(const glm::vec2& i_anchor) {
		assert_handle();
		set_dirty(anchor != i_anchor);
		anchor = i_anchor;
	}

	glm::vec2 Draw_2D::get_position() const {
		assert_handle();
		return position;
	}

	float Draw_2D::get_rotation() const {
		assert_handle();
		return rotation;
	}

	glm::vec2 Draw_2D::get_anchor() const {
		assert_handle();
		return anchor;
	}

	Draw_2D_Attributes_Description Draw_2D::get_description() const {
		Draw_2D_Attributes_Description description{};
		static_cast<Draw_Attributes_Description&>(description) = Draw_Atributes::get_description();
		description.position = position;
		description.rotation = rotation;
		description.anchor = anchor;
		return description;
	}

	Draw_2D_Scale_Aable::Draw_2D_Scale_Aable() {}

	Draw_2D_Scale_Aable::Draw_2D_Scale_Aable(const Handle& handle, const Draw_2D_Scale_Able_Description& description)
		: Draw_2D(handle, description) {
		scale = description.scale;
	}

	Draw_2D_Scale_Aable::~Draw_2D_Scale_Aable() {}

	void Draw_2D_Scale_Aable::set_scale(const glm::vec2& i_scale) {
		assert_handle();
		set_dirty(scale != i_scale);
		scale = i_scale;
	}

	glm::vec2 Draw_2D_Scale_Aable::get_scale() const {
		assert_handle();
		return scale;
	}

	Draw_2D_Scale_Able_Description Draw_2D_Scale_Aable::get_description() const {
		return {Draw_2D::get_description(), scale};
	}

	Font_2D::Font_2D() {
		reload_text_size();
	}

	Font_2D::Font_2D(const Handle& handle, const Label_2D_Description& description)
		: Draw_2D_Scale_Aable(handle, description) {
		path = description.path;
		text = description.text;
		font_size = description.font_size;
		align = description.align;
		stroke_color = description.stroke_color;
		stroke_size = description.stroke_size;
	}

	Font_2D::~Font_2D() {}

	void Font_2D::reload_text_size() {
		if (text_dirty) {
			auto text_helper = Draw_Font_2D::gen_text_info_from(path, text, align, font_size, stroke_size);
			width = text_helper.width;
			height = text_helper.height;
			text_dirty = false;
		}
	}

	void Font_2D::set_font_path(const std::string& i_path) {
		assert_handle();
		set_dirty(path != i_path);
		text_dirty |= (path != i_path);
		path = i_path;
	}

	void Font_2D::set_text(const std::string& i_text) {
		assert_handle();
		set_dirty(text != i_text);
		text_dirty |= (text != i_text);
		text = i_text;
	}

	void Font_2D::set_font_size(int i_font_size) {
		assert_handle();
		set_dirty(font_size != i_font_size);
		text_dirty |= (font_size != i_font_size);
		font_size = i_font_size;
	}

	void Font_2D::set_align(int i_align) {
		assert_handle();
		set_dirty(align != i_align);
		align = i_align;
	}

	void Font_2D::set_width(float i_width) {
		width = i_width;
	}

	void Font_2D::set_height(float i_height) {
		height = i_height;
	}

	void Font_2D::set_stroke_size(float i_size) {
		assert_handle();
		set_dirty(stroke_size != i_size);
		text_dirty |= (stroke_size != i_size);
		stroke_size = i_size;
	}

	void Font_2D::set_stroke_color(const glm::vec3& i_color) {
		assert_handle();
		set_dirty(stroke_color != i_color);
		stroke_color = i_color;
	}

	std::string Font_2D::get_font_path() const {
		assert_handle();
		return path;
	}

	std::string Font_2D::get_text() const {
		assert_handle();
		return text;
	}

	int Font_2D::get_font_size() const {
		assert_handle();
		return font_size;
	}

	int Font_2D::get_align() const {
		assert_handle();
		return align;
	}

	float Font_2D::get_stroke_size() const {
		assert_handle();
		return stroke_size;
	}

	glm::vec3 Font_2D::get_stroke_color() const {
		assert_handle();
		return stroke_color;
	}

	float Font_2D::get_width() {
		assert_handle();
		reload_text_size();
		return width;
	}

	float Font_2D::get_height() {
		assert_handle();
		reload_text_size();
		return height;
	};

	Label_2D_Description Font_2D::get_description() const {
		Label_2D_Description description{};
		static_cast<Draw_2D_Attributes_Description&>(description) = Draw_2D::get_description();
		description.path = path;
		description.text = text;
		description.font_size = font_size;
		description.align = align;
		return description;
	}

	Texture_2D::Texture_2D() {}

	Texture_2D::Texture_2D(const Handle& handle, const Texture_2D_Description& description)
		: Draw_2D_Scale_Aable(handle, description) {
		path = description.path;
		tex_rect = description.tex_rect;
	}

	Texture_2D::~Texture_2D() {}

	void Texture_2D::update_texture_size() {
		if (texture_dirty) {
			auto texture_helper = Draw_Texture_2D::gen_texture_info_from(path);
			width = texture_helper.width * tex_rect.ratio_width;
			height = texture_helper.height * tex_rect.ratio_height;
			texture_dirty = true;
		}
	}

	void Texture_2D::set_texture_path(const std::string& i_path) {
		assert_handle();
		set_dirty(path != i_path);
		texture_dirty |= (path != i_path);
		path = i_path;
	}

	void Texture_2D::set_texture_rect(const Geometry::Texture_Rect_2D& i_tex_rect) {
		assert_handle();
		set_dirty(tex_rect != i_tex_rect);
		texture_dirty |= (tex_rect != i_tex_rect);
		tex_rect = i_tex_rect;
	}

	float Texture_2D::get_width() {
		assert_handle();
		update_texture_size();
		return width;
	}

	float Texture_2D::get_height() {
		assert_handle();
		update_texture_size();
		return height;
	}

	std::string Texture_2D::get_texture_path() const {
		assert_handle();
		return path;
	}

	Geometry::Texture_Rect_2D Texture_2D::get_texture_rect() const {
		assert_handle();
		return tex_rect;
	}

	Texture_2D_Description Texture_2D::get_description() const {
		return {Draw_2D_Scale_Aable::get_description(), path, tex_rect};
	}

	Rectangle_2D::Rectangle_2D() {}

	Rectangle_2D::Rectangle_2D(const Handle& handle, const Rectangle_2D_Description& description)
		: Draw_2D(handle, description) {
		width = description.width;
		height = description.height;
		border_width = description.border_width;
		border_radius = description.border_radius;
		border_color = description.border_color;
	}

	Rectangle_2D::~Rectangle_2D() {}

	void Rectangle_2D::set_width(const float& i_width) {
		assert_handle();
		set_dirty(width != i_width);
		width = i_width;
	}

	void Rectangle_2D::set_height(const float& i_height) {
		assert_handle();
		set_dirty(height != i_height);
		height = i_height;
	}

	void Rectangle_2D::set_border_radius(const float& i_border_radius) {
		assert_handle();
		set_dirty(border_radius != i_border_radius);
		border_radius = i_border_radius;
	}

	void Rectangle_2D::set_border_width(const float& i_border_width) {
		assert_handle();
		set_dirty(border_width != i_border_width);
		border_width = i_border_width;
	}

	void Rectangle_2D::set_border_color(const glm::vec3& i_color) {
		assert_handle();
		set_dirty(border_color != i_color);
		border_color = i_color;
	}

	float Rectangle_2D::get_width() const {
		assert_handle();
		return width;
	}

	float Rectangle_2D::get_height() const {
		assert_handle();
		return height;
	}

	float Rectangle_2D::get_border_radius() const {
		assert_handle();
		return border_radius;
	}

	float Rectangle_2D::get_border_width() const {
		assert_handle();
		return border_width;
	}

	glm::vec3 Rectangle_2D::get_border_color() const {
		assert_handle();
		return border_color;
	}

	Rectangle_2D_Description Rectangle_2D::get_description() const {
		return {Draw_2D::get_description(), width, height, border_radius, border_width, border_color};
	}

	Triangle_2D::Triangle_2D() {}

	Triangle_2D::Triangle_2D(const Handle& handle, const Triangle_2D_Description& description)
		: Draw_Atributes(handle, description) {
		first = description.first;
		second = description.second;
		third = description.third;
	}

	Triangle_2D::~Triangle_2D() {}

	void Triangle_2D::set_first_point(const glm::vec2& i_first) {
		assert_handle();
		set_dirty(first != i_first);
		first = i_first;
	}

	void Triangle_2D::set_second_point(const glm::vec2& i_second) {
		assert_handle();
		set_dirty(second != i_second);
		second = i_second;
	}

	void Triangle_2D::set_third_point(const glm::vec2& i_third) {
		assert_handle();
		set_dirty(third != i_third);
		third = i_third;
	}

	glm::vec2 Triangle_2D::get_first_point() const {
		assert_handle();
		return first;
	}

	glm::vec2 Triangle_2D::get_second_point() const {
		assert_handle();
		return second;
	}

	glm::vec2 Triangle_2D::get_third_point() const {
		assert_handle();
		return third;
	}

	Triangle_2D_Description Triangle_2D::get_description() const {
		return {Draw_Atributes::get_description(), first, second, third};
	}

	Clipping_2D::Clipping_2D() {}

	Clipping_2D::Clipping_2D(const Handle& handle, const Clipping_Description& description)
		: Base_Attributes(handle) {
		size = description.size;
		border_size = description.border_size;
		world_position = description.world_position;
		world_scale = description.world_scale;
		world_rotation = description.world_rotation;
	}

	Clipping_2D::~Clipping_2D() {}

	void Clipping_2D::set_content_size(const glm::vec2& i_size) {
		assert_handle();
		set_dirty(size != i_size);
		size = i_size;
	}

	void Clipping_2D::set_border_radius(float border_radius) {
		assert_handle();
		set_dirty(border_size != border_radius);
		border_size = border_radius;
	}

	void Clipping_2D::set_world_position(const glm::vec2& i_world_position) {
		assert_handle();
		set_dirty(world_position != i_world_position);
		world_position = i_world_position;
	}

	void Clipping_2D::set_world_scale(const glm::vec2& i_world_scale) {
		assert_handle();
		set_dirty(world_scale != i_world_scale);
		world_scale = i_world_scale;
	}

	void Clipping_2D::set_world_rotation(float i_rotation) {
		assert_handle();
		set_dirty(world_rotation != i_rotation);
		world_rotation = i_rotation;
	}

	void Clipping_2D::set_anchor(const glm::vec2& i_anchor) {
		assert_handle();
		set_dirty(anchor != i_anchor);
		anchor = i_anchor;
	}

	void Clipping_2D::set_border_width(float i_border_width) {
		assert_handle();
		set_dirty(border_width != i_border_width);
		border_width = i_border_width;
	}

	glm::vec2 Clipping_2D::get_content_size() const {
		assert_handle();
		return size;
	}

	float Clipping_2D::get_border_radius() const {
		assert_handle();
		return border_size;
	}

	glm::vec2 Clipping_2D::get_world_position() const {
		assert_handle();
		return world_position;
	}

	glm::vec2 Clipping_2D::get_world_scale() const {
		assert_handle();
		return world_scale;
	}

	float Clipping_2D::get_world_rotation() const {
		assert_handle();
		return world_rotation;
	}

	glm::vec2 Clipping_2D::get_anchor() const {
		assert_handle();
		return anchor;
	}

	float Clipping_2D::get_border_width() const {
		assert_handle();
		return border_width;
	}

	Clipping_Description Clipping_2D::get_description() const {
		return {size, border_size, border_width, world_position, world_scale, anchor, world_rotation};
	}

} // namespace Vulkan
