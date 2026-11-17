// [Claude 2026-11-17] whole file: GLFW -> engine input enum converters (enums in engine/structs.h).
// No GLFW include here on purpose; only input_glfw.cpp sees GLFW. Another backend = another file
// (input_android.h / .cpp with android_to_key, ...), the enums and the rest of the engine stay the same.
#pragma once
#include <engine/structs.h>

namespace Engine {

	/**
	 * @Note: GLFW_KEY_* -> KEY. Unknown / out of range -> KEY_UNKNOWN.
	 */
	KEY glfw_to_key(int key);

	/**
	 * @Note: GLFW_RELEASE / GLFW_PRESS / GLFW_REPEAT -> KEY_ACTION. Anything else -> KEY_ACTION_RELEASE.
	 * Also valid for mouse button actions (GLFW only sends RELEASE / PRESS there).
	 */
	KEY_ACTION glfw_to_key_action(int action);

	/**
	 * @Note: GLFW_MOUSE_BUTTON_* -> MOUSE_BUTTON. Out of range -> MOUSE_BUTTON_UNKNOWN.
	 */
	MOUSE_BUTTON glfw_to_mouse_button(int button);

	/**
	 * @Note: GLFW_MOD_* bits -> KEY_MOD_* bits (unknown bits dropped). Test with (mods & KEY_MOD_SHIFT).
	 */
	int glfw_to_key_mods(int mods);

} // namespace Engine
