#include <editor_api.h>
#include <math_custom.h>

namespace Editor_Api {

	glm::vec2 to_child_space(const Handle& e, const glm::vec2& world_position) {
		return Engine::to_my_position_space(e, world_position);
	}

	bool is_point_in_node(const Handle& e, const glm::vec2& world_position) {
		if (!Engine::is_valid(e))
			return false;
		return Math::point_in_rect(Engine::get_world_rect(e), world_position);
	}

} // namespace Editor_Api
