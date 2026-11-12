#pragma once
#include <GLFW/glfw3.h>
#include <ecs.h>
#include <engine/structs.h>
#include <geometry_structs.h>
#include <handle.h>

namespace Engine {

	// Engine consts.

	const uint64_t DIRTY_TRANSFORM = (1 << 0);

	const uint64_t DIRTY_RENDERER = (1 << 1);

	/**
	 * @Note: if this flag on the size relative
	 * with parent does not resovle to raw yet!
	 */
	const uint64_t DIRTY_RELATIVE_SIZE = (1 << 2);

	/**
	 * @Note: if this flag on the position relative
	 * with size parent does not resovle to raw yet!
	 */
	const uint64_t DIRTY_RELATIVE_POSITION = (1 << 3);

	const uint64_t DIRTY_CLIPPING = (1 << 4);

	/**
	 * @Note: from now each time change other will
	 * loop all tree next time will implement
	 * buffer nodes to each child.
	 */
	const uint64_t DIRTY_DRAW_ORDER = (1 << 5);

	const uint64_t DIRTY_TOUCHING = (1 << 6);

	const uint64_t DIRTY_CONTAINER_TRANSFORM = (1 << 7);

	const uint64_t DIRTY_SCROLLING = (1 << 8);

	// Main caller functions.
	void init(GLFWwindow* window);

	void process_frame(double dt);

	void destroy();

	glm::vec2 get_window_size();

	Registry& get_registry();

	// Create/Remove/Handle objects.

	bool is_valid(const Handle& e);

	Handle create_node();

	Handle create_layout();

	Handle create_image();

	Handle create_label();

	Handle create_scroll_layout();

	Handle get_parent(const Handle& e);

	std::vector<Handle> get_children(const Handle& e);

	std::vector<Handle> get_scroll_layout_children(const Handle& e);

	void add_child(const Handle& e_parent, const Handle& e_children);

	void add_child_to_container(const Handle& e_parent, const Handle& e_child);

	void remove_child(const Handle& e_child);

	void clean_up(const Handle& e);

	// Helper functions.

	void set_dirty(const Handle& e, bool dirty, uint64_t flag);

	void clean_dirty(const Handle& e, uint64_t flag);

	bool is_dirty(const Handle& e, uint64_t flag);

	// Query/Update objects.

	bool is_world_position_in_entity_rect(const Handle& e, const glm::vec2& touch_position);

	bool is_scroll_position_valid_with(const Handle& e, const glm::vec2& scroll_position);

	std::vector<Handle> find_entites_listen_touch_at(const glm::vec2& world_touch_position);

	std::vector<Handle> find_entities_listen_scroll_at(const glm::vec2& world_touch_position);

	const Handle& get_root();

	void set_scale(const Handle& e, const glm::vec2& scale);

	glm::vec2 get_scale(const Handle& e);

	void set_position(const Handle& e, const glm::vec2& position);

	glm::vec2 get_position(const Handle& e);

	void set_rotation(const Handle& e, float rotation);

	float get_rotation(const Handle& e);

	void set_opacity(const Handle& e, float opacity);

	float get_opacity(const Handle& e);

	void set_color(const Handle& e, const glm::vec3& color);

	glm::vec3 get_color(const Handle& e);

	void set_anchor(const Handle& e, const glm::vec2& anchor);

	glm::vec2 get_anchor(const Handle& e);

	void set_visible(const Handle& e, bool is_visible);

	bool is_visible(const Handle& e);

	void set_z_order(const Handle& e, uint32_t z_order);

	uint32_t get_z_order(const Handle& e);

	void set_name(const Handle& e, const std::string& name);

	std::string get_name(const Handle& e);

	void set_tag(const Handle& e, int tag);

	int get_tag(const Handle& e);

	void set_touch_enabled(const Handle& e, bool touch_enabled);

	bool is_touch_enabled(const Handle& e);

	void set_swallow_touches(const Handle& e, bool swallows);

	bool is_swallow_touches(const Handle& e);

	void set_touch_caller(const Handle& e, const std::function<void(Handle, TOUCH_STATE, glm::vec2, int, int)> caller);

	void set_click_caller(const Handle& e, const std::function<void(Handle, glm::vec2, int, int)> caller);

	void set_nine_scale_enabled(const Handle& e, bool enabled);

	bool is_nine_scale_enabled(const Handle& e);

	void set_nine_scale_rect(const Handle& e, const Geometry::Rect_2D& capset);

	Geometry::Rect_2D get_nine_scale_rect(const Handle& e);

	void set_image_nine_scale_size(const Handle& e, const glm::vec2& renderer_size);

	glm::vec2 get_image_nine_scale_size(const Handle& e, const Handle& renderer_size);

	void set_nine_scale_rect_ratio(const Handle& e, const Geometry::Texture_Rect_2D& capset_ratio);

	Geometry::Texture_Rect_2D get_nine_scale_rect_ratio(const Handle& e);

	void set_content_size(const Handle& e, const glm::vec2& size);

	glm::vec2 get_content_size(const Handle& e);

	glm::vec2 get_inner_renderer_size(const Handle& e);

	void set_position_percent(const Handle& e, const glm::vec2& ratio_position);

	glm::vec2 get_position_percent(const Handle& e);

	void set_size_percent(const Handle& e, const glm::vec2& ratio_size);

	glm::vec2 get_size_percent(const Handle& e);

	void set_size_pinning(const Handle& e, const Size_Pinning_Data& pinning);

	Size_Pinning_Data get_size_pinning(const Handle& e);

	glm::vec2 get_world_position(const Handle& e);

	glm::vec2 to_my_position_space(
		const Handle& e, const glm::vec2& world_position, const glm::vec2& fallback_position = {0.f, 0.f}
	);

	std::array<glm::vec2, 4> get_world_rect(const Handle& e);

	bool is_world_visible(const Handle& e);

	void set_layout_background_enabled(const Handle& e, bool enabled);

	bool is_layout_background_enabled(const Handle& e);

	void set_layout_background_color(const Handle& e, const glm::vec3& color);

	glm::vec3 get_layout_background_color(const Handle& e);

	void set_layout_border_radius(const Handle& e, float border_radius);

	float get_layout_border_radius(const Handle& e);

	void set_layout_border_width(const Handle& e, float border_width);

	float get_layout_border_width(const Handle& e);

	void set_layout_border_color(const Handle& e, const glm::vec3& color);

	glm::vec3 get_layout_border_color(const Handle& e);

	void set_layout_clipping_enabled(const Handle& e, bool enabled);

	bool is_layout_clipping_enabled(const Handle& e);

	void set_image_path(const Handle& e, const std::string& image_path);

	std::string get_image_path(const Handle& e);

	void set_image_rect(const Handle& e, const Geometry::Texture_Rect_2D& image_rect);

	void set_image_fit_mode(const Handle& e, IMAGE_RENDER_FIT_MODE fit_mode);

	IMAGE_RENDER_FIT_MODE get_image_fit_mode(const Handle& e);

	Geometry::Texture_Rect_2D get_image_rect(const Handle& e);

	void set_label_font_size(const Handle& e, int font_size);

	int get_label_font_size(const Handle& e);

	void set_label_font_path(const Handle& e, const std::string& font_path);

	std::string get_label_font_path(const Handle& e);

	void set_label_string(const Handle& e, const std::string& text);

	std::string get_label_string(const Handle& e);

	void set_label_stroke_size(const Handle& e, float stroke_size);

	float get_label_stroke_size(const Handle& e);

	void set_label_stroke_color(const Handle& e, const glm::vec3& color);

	glm::vec3 get_label_stroke_color(const Handle& e);

	void set_scroll_enabled(const Handle& e, bool enabled);

	bool is_scroll_enabled(const Handle& e);

	void set_inner_container_size(const Handle& e, const glm::vec2& size);

	glm::vec2 get_inner_container_size(const Handle& e);

	void set_scroll_direction(const Handle& e, SCROLL_DIRECTION direction);

	SCROLL_DIRECTION get_scroll_direction(const Handle& e);

	void set_scroll_speed(const Handle& e, float speed);

	float get_scroll_speed(const Handle& e);

	void set_scroll_dragging_threshold(const Handle& e, float threshold = 7.5f);

	float get_scroll_dragging_threshold(const Handle& e);

	void set_layout_scroll_touch_enabled(const Handle& e, bool enabled);

	bool is_layout_scroll_touch_enabled(const Handle& e);

	void set_swallow_scrolling(const Handle& e, bool swallows);

	bool is_swallow_scrolling(const Handle& e);

	void set_scroll_layout_container_position(const Handle& e, const glm::vec2& inner_container_position);

	glm::vec2 get_scroll_layout_container_position(const Handle& e);

	Handle find_node_by_name(const std::string& name, Handle parent = Handle::make_invalid_handle());

	Handle find_node_by_tag(int tag, Handle parent = Handle::make_invalid_handle());

	void set_label_horizontal_align(const Handle& e, LABEL_HORIZONTAL_ALIGN horizontal_align);

	LABEL_HORIZONTAL_ALIGN get_label_horizontal_align(const Handle& e);

	void set_label_vertical_align(const Handle& e, LABEL_VERTICAL_ALIGN vertical_align);

	LABEL_VERTICAL_ALIGN get_label_vertical_align(const Handle& e);

	void set_label_padding_horizontal(const Handle& e, float padding_horizontal);

	float get_label_padding_horizontal(const Handle& e);

	void set_label_padding_vertical(const Handle& e, float padding_vertical);

	float get_label_padding_vertical(const Handle& e);

	void set_label_render_fit_mode(const Handle& e, LABEL_RENDER_FIT_MODE fit_mode);

	LABEL_RENDER_FIT_MODE get_label_render_fit_mode(const Handle& e);

	int get_min_font_size(const std::string& path);

	int get_max_font_size(const std::string& path);

	void set_clear_screen_color(const glm::vec3& color, float opacity = 1.f);

	glm::vec2 query_image_size(const std::string& path);

	/**
	 * @Note: when this function call all
	 * other entity is touching beside
	 * this entity will be canceled.
	 */
	void claim_touch(const Handle& e);

	glm::vec2 get_cursor_world_position();

	void set_touch_cancel_on_fast_move(const Handle& e, bool enabled);

	Handle find_topmost_node_at(const glm::vec2& world_position, Handle under = get_root());

} // namespace Engine