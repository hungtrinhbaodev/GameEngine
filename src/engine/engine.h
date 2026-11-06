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

	// Main caller functions.
	void init(GLFWwindow* window);

	void process_frame(double dt);

	void destroy();

	glm::vec2 get_window_size();

	Registry& get_registry();

	// Create/Remove/Handle objects.

	Handle create_node();

	Handle create_layout();

	Handle create_image();

	Handle create_label();

	void add_child(const Handle& e_parent, const Handle& e_children);

	void remove_child(const Handle& e_child);

	void clean_up(const Handle& e);

	// Helper functions.

	/**
	 * @Note: if wanna recalculate all
	 * percent size or pin size into raw
	 * size call this to node it will
	 * parse all size relate with parent
	 * into raw size to use.
	 *
	 * @Condition: node need to have
	 * parent before. if node doesn't
	 * have parent resolve will be fail.
	 *
	 * @Apply: this node and all child.
	 */
	void resovle_relative_size(const Handle& e);

	/**
	 * @Note: if wanna recalculate all
	 * position percent to raw position
	 * call this to entity.
	 *
	 * @Condition: node need to have
	 * parent before. if node doesn't
	 * have parent resolve will be fail.
	 *
	 * @Apply: this node and all child.
	 */
	void resovle_relative_position(const Handle& e);

	void set_dirty(const Handle& e, bool dirty, uint64_t flag);

	void clean_dirty(const Handle& e, uint64_t flag);

	bool is_dirty(const Handle& e, uint64_t flag);

	// Query/Update objects.

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

	void set_content_size(const Handle& e, const glm::vec2& size);

	glm::vec2 get_content_size(const Handle& e);

	void set_position_percent(const Handle& e, const glm::vec2& ratio_position);

	glm::vec2 get_position_percent(const Handle& e);

	void set_size_percent(const Handle& e, const glm::vec2& ratio_size);

	glm::vec2 get_size_percent(const Handle& e);

	void set_size_pinning(const Handle& e, const Size_Pinning_Data& pinning);

	Size_Pinning_Data get_size_pinning(const Handle& e);

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

} // namespace Engine