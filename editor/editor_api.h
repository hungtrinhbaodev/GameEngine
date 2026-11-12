#pragma once
#include <engine/engine.h>
#include <glm/glm.hpp>
#include <string>

/**
 * @Note: the editor's remaining stand-ins for engine behaviour that is missing or buggy.
 * Everything else the editor needs now comes straight from Engine:: (query_image_size, claim_touch,
 * get_cursor_world_position, set_touch_cancel_on_fast_move, set_layout_scroll_touch_enabled,
 * set_clear_screen_color). When an item below lands, make the body a one-line forward.
 */
namespace Editor_Api {

	/**
	 * @Note: the point in the space e's children are positioned in. Fixed in the engine (#25): for layouts the
	 * origin is the bottom-left (with rotation / scale), for other nodes the anchor point. Kept as a name only.
	 */
	glm::vec2 to_child_space(const Handle& e, const glm::vec2& world_position);

	// world point inside e's world rect (no clip / visibility rules).
	bool is_point_in_node(const Handle& e, const glm::vec2& world_position);

} // namespace Editor_Api
