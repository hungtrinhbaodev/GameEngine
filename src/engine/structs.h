#pragma once
#include <glm/glm.hpp>
#include <handle.h>

namespace Engine {

	struct Size_Pinning_Data {
		float left = 0.f, right = 0.f, top = 0.f, bottom = 0.f;
	};

	enum TOUCH_STATE {
		TOUCH_BEGAN,
		TOUCH_MOVE,
		TOUCH_ENDED,
		TOUCH_CANCEL,
		TOUCH_INIT
	};

	enum SCROLL_DIRECTION {
		SCROLL_HORIZONTAL,
		SCROLL_VERTICAL
	};

	struct Handled_Touches_Information {
		glm::vec2 touch_world_position{0.f};
		TOUCH_STATE touch_state = TOUCH_STATE::TOUCH_INIT;
		std::vector<Handle> entities;
	};

} // namespace Engine