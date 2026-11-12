#pragma once
#include <glm/glm.hpp>
#include <handle.h>

namespace Engine {

	struct Size_Pinning_Data {
		float left = 0.f, right = 0.f, top = 0.f, bottom = 0.f;
	};

	enum TOUCH_STATE {
		/**
		 * @Note: user using state.
		 */
		TOUCH_BEGAN,
		TOUCH_MOVE,
		TOUCH_ENDED,
		TOUCH_CANCEL,
		/**
		 * @Note: logic fill user do
		 * not use this flags.
		 */
		TOUCH_INIT,
		TOUCH_PENDING_RELEASE,
	};

	enum SCROLL_DIRECTION {
		SCROLL_HORIZONTAL,
		SCROLL_VERTICAL
	};

	enum TOUCH_SCROLL_STATE {
		IDLE,
		SCROLLING
	};

	enum LABEL_VERTICAL_ALIGN {
		LABEL_VERTICAL_ALIGN_TOP,
		LABEL_VERTICAL_ALIGN_CENTER,
		LABEL_VERTICAL_ALIGN_BOTTOM
	};

	enum LABEL_HORIZONTAL_ALIGN {
		LABEL_HORIZONTAL_ALIGN_CENTER,
		LABEL_HORIZONTAL_ALIGN_LEFT,
		LABEL_HORIZONTAL_ALIGN_RIGHT
	};

	enum LABEL_RENDER_FIT_MODE {
		LABEL_RENDER_FIT_AUTO,
		LABEL_RENDER_FIT_CLIP,
		LABEL_RENDER_FIT_ELLIPSIS
	};

	enum IMAGE_RENDER_FIT_MODE {
		IMAGE_RENDER_FIT_AUTO,
		IMAGE_RENDER_FIT_STRETCH,
		IMAGE_RENDER_FIT_FIT,
		IMAGE_RENDER_FIT_FILL
	};

	enum CLIPPING_STATE {
		CLIPPING_FULL_OUT,
		CLIPPING_FULL_IN,
		CLIPPING_INTERECT
	};

	struct Handled_Touches_Information {
		glm::vec2 touch_world_position{0.f};
		TOUCH_STATE touch_state = TOUCH_STATE::TOUCH_INIT;
		bool remove_next_frame = false;
		std::vector<Handle> entities;
	};

} // namespace Engine