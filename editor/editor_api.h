#pragma once
#include <engine/engine.h>
#include <glm/glm.hpp>
#include <string>

struct GLFWwindow;

/**
 * @Note: stand-ins for engine APIs the editor needs but the engine does not have yet.
 * Each one says which engine API should replace it; once that lands, make the body a
 * one-line forward (or delete the function and call the engine directly).
 */
namespace Editor_Api {

	void init(GLFWwindow* window);

	/**
	 * @Note: TODO(engine) glm::vec2 Engine::get_texture_size(const std::string& image_path).
	 * Stand-in: loads the texture through Vulkan::load_texture and reads its view.
	 * Returns {0, 0} when the file does not exist.
	 */
	glm::vec2 get_texture_size(const std::string& image_path);

	/**
	 * @Note: TODO(engine) glm::vec2 Engine::get_cursor_world_position().
	 * Stand-in: glfwGetCursorPos, flipped to the engine's bottom-left origin.
	 */
	glm::vec2 get_cursor_world_position();

	/**
	 * @Note: TODO(engine) Handle Engine::find_topmost_node_at(world_position, under).
	 * Stand-in for the one case the editor needs today: is the point inside e's world rect.
	 */
	bool is_point_in_node(const Handle& e, const glm::vec2& world_position);

	/**
	 * @Note: TODO(engine) Engine::to_my_position_space(e, world) should already return this: a point in the
	 * space e's children are positioned in (origin = e's bottom-left). Today it is measured from e's anchor
	 * point, so for a node with anchor (0.5, 0.5) its centre maps to (0, 0) = the children's bottom-left.
	 * Stand-in: engine result + anchor * content size.
	 */
	glm::vec2 to_child_space(const Handle& e, const glm::vec2& world_position);

	/**
	 * @Note: TODO(engine) Engine::set_touch_layout_scroll_enabled(e, false) should do this, but it switches
	 * touch on the inner container (Scroll_Information::container_handle) instead of the touch listener
	 * container (container_touch_handle), so the layout still scrolls by drag. Stand-in: switch off the
	 * listener container found among the scroll layout's children (Description SCROLL_LISTENER_CONTAINER).
	 */
	void set_scroll_drag_enabled(const Handle& scroll_layout, bool enabled);

	/**
	 * @Note: TODO(engine) void Engine::claim_touch(const Handle& e).
	 * Not possible from outside the engine. Workaround in the editor: the resource panel
	 * scrolls with the mouse wheel only (touch scrolling off), so dragging a cell never
	 * fights the scroll layout.
	 */

} // namespace Editor_Api
