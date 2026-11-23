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
		LABEL_RENDER_FIT_ELLIPSIS,
		LABEL_RENDER_FIT_WIDTH_SHOW_ALL,
		LABEL_RENDER_FIT_HEIGHT_SHOW_ALL
	};

	enum LABEL_RENDER_STRING_TYPE {
		LABEL_RENDER_STRING_TYPE_RAW,
		LABEL_RENDER_STRING_TYPE_TRIM_SPACE
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

	enum POSITION_TYPE {
		POSITION_TYPE_ABSOLUTE,
		POSITION_TYPE_PERCENT_RELATIVE
	};

	enum SIZE_TYPE {
		SIZE_TYPE_ABSOLUTE,
		SIZE_TYPE_PERCENT_RELATIVE,
		SIZE_TYPE_PINNING_RELATIVE
	};

	enum SIDE_FIX_MODE {
		SIZE_FIX_WIDTH,
		SIZE_FIX_HEIGHT
	};

	enum DESCRIPTION_TYPE {
		NONE,
		NODE_2D,
		LAYOUT_2D,
		IMAGE_2D,
		LABEL_2D,
		SCROLL_LAYOUT,
		INNER_CONTAINER,
		SCROLL_LISTENER_CONTAINER,
		EDIT_BOX,
	};

	enum EDIT_BOX_STATE {
		EDIT_BOX_STATE_IDLE,
		EDIT_BOX_STATE_FOCUSED
	};

	// [Claude 2026-11-17] BEGIN input enums: the engine API speaks these, not backend numbers, so swapping the window
	// backend only needs its own converter file. Convert with engine/input_glfw.h (glfw_to_key, glfw_to_key_action,
	// glfw_to_mouse_button, glfw_to_key_mods); a new backend adds e.g. input_android.h with android_to_key.
	// Values are our own (0, 1, 2, ...); never store them in files or rely on them matching any backend.

	enum KEY_ACTION {
		KEY_ACTION_RELEASE,
		KEY_ACTION_PRESS,
		/**
		 * @Note: held key auto-repeat; treat like PRESS for Backspace / Delete / arrows.
		 */
		KEY_ACTION_REPEAT,
	};

	enum MOUSE_BUTTON {
		/**
		 * @Note: backend sent a button we do not know (converter output only).
		 */
		MOUSE_BUTTON_UNKNOWN,
		MOUSE_BUTTON_LEFT,
		MOUSE_BUTTON_RIGHT,
		MOUSE_BUTTON_MIDDLE,
		MOUSE_BUTTON_4,
		MOUSE_BUTTON_5,
		MOUSE_BUTTON_6,
		MOUSE_BUTTON_7,
		MOUSE_BUTTON_8,
		/**
		 * @Note: number of buttons, keep last (array size for per-button state).
		 */
		MOUSE_BUTTON_COUNT,
	};

	/**
	 * @Note: bit flags, a mods int can hold several: (mods & KEY_MOD_SUPER) != 0.
	 * Keep the explicit values: they are our own bits (one bit per key), not backend numbers.
	 * SUPER = Cmd on macOS, Windows key on Windows.
	 */
	enum KEY_MODIFIER {
		KEY_MOD_SHIFT = 1 << 0,
		KEY_MOD_CONTROL = 1 << 1,
		KEY_MOD_ALT = 1 << 2,
		KEY_MOD_SUPER = 1 << 3,
		KEY_MOD_CAPS_LOCK = 1 << 4,
		KEY_MOD_NUM_LOCK = 1 << 5,
	};

	/**
	 * @Note: physical keys (US layout names). Typed text never comes from here, it comes from the
	 * char callback as char32_t (layout, Shift and Telex already applied).
	 */
	enum KEY {
		KEY_UNKNOWN,
		KEY_SPACE,
		KEY_APOSTROPHE,
		KEY_COMMA,
		KEY_MINUS,
		KEY_PERIOD,
		KEY_SLASH,
		KEY_0,
		KEY_1,
		KEY_2,
		KEY_3,
		KEY_4,
		KEY_5,
		KEY_6,
		KEY_7,
		KEY_8,
		KEY_9,
		KEY_SEMICOLON,
		KEY_EQUAL,
		KEY_A,
		KEY_B,
		KEY_C,
		KEY_D,
		KEY_E,
		KEY_F,
		KEY_G,
		KEY_H,
		KEY_I,
		KEY_J,
		KEY_K,
		KEY_L,
		KEY_M,
		KEY_N,
		KEY_O,
		KEY_P,
		KEY_Q,
		KEY_R,
		KEY_S,
		KEY_T,
		KEY_U,
		KEY_V,
		KEY_W,
		KEY_X,
		KEY_Y,
		KEY_Z,
		KEY_LEFT_BRACKET,
		KEY_BACKSLASH,
		KEY_RIGHT_BRACKET,
		KEY_GRAVE_ACCENT,
		KEY_WORLD_1,
		KEY_WORLD_2,
		KEY_ESCAPE,
		KEY_ENTER,
		KEY_TAB,
		KEY_BACKSPACE,
		KEY_INSERT,
		KEY_DELETE,
		KEY_RIGHT,
		KEY_LEFT,
		KEY_DOWN,
		KEY_UP,
		KEY_PAGE_UP,
		KEY_PAGE_DOWN,
		KEY_HOME,
		KEY_END,
		KEY_CAPS_LOCK,
		KEY_SCROLL_LOCK,
		KEY_NUM_LOCK,
		KEY_PRINT_SCREEN,
		KEY_PAUSE,
		KEY_F1,
		KEY_F2,
		KEY_F3,
		KEY_F4,
		KEY_F5,
		KEY_F6,
		KEY_F7,
		KEY_F8,
		KEY_F9,
		KEY_F10,
		KEY_F11,
		KEY_F12,
		KEY_F13,
		KEY_F14,
		KEY_F15,
		KEY_F16,
		KEY_F17,
		KEY_F18,
		KEY_F19,
		KEY_F20,
		KEY_F21,
		KEY_F22,
		KEY_F23,
		KEY_F24,
		KEY_F25,
		KEY_KP_0,
		KEY_KP_1,
		KEY_KP_2,
		KEY_KP_3,
		KEY_KP_4,
		KEY_KP_5,
		KEY_KP_6,
		KEY_KP_7,
		KEY_KP_8,
		KEY_KP_9,
		KEY_KP_DECIMAL,
		KEY_KP_DIVIDE,
		KEY_KP_MULTIPLY,
		KEY_KP_SUBTRACT,
		KEY_KP_ADD,
		KEY_KP_ENTER,
		KEY_KP_EQUAL,
		KEY_LEFT_SHIFT,
		KEY_LEFT_CONTROL,
		KEY_LEFT_ALT,
		KEY_LEFT_SUPER,
		KEY_RIGHT_SHIFT,
		KEY_RIGHT_CONTROL,
		KEY_RIGHT_ALT,
		KEY_RIGHT_SUPER,
		KEY_MENU,
		/**
		 * @Note: number of keys, keep last (array size for per-key state).
		 */
		KEY_COUNT,
	};
	// [Claude 2026-11-17] END input enums

	struct Handled_Touches_Information {
		Handle id = Handle::make_invalid_handle();
		glm::vec2 touch_world_position{0.f};
		TOUCH_STATE touch_state = TOUCH_STATE::TOUCH_INIT;
		bool remove_next_frame = false;
		MOUSE_BUTTON touch_type = MOUSE_BUTTON::MOUSE_BUTTON_LEFT;
		int touch_mods = -1;
	};

	struct Handled_Key_Information {
		KEY key = KEY::KEY_UNKNOWN;
		KEY_ACTION key_action = KEY_ACTION::KEY_ACTION_PRESS;
		int key_mods = -1;
	};

} // namespace Engine