#include <engine/input_glfw.h>
#include <engine/key_handler.h>

namespace Engine {

	std::vector<Handled_Key_Information> handled_keys;

	std::vector<char32_t> handled_chars;

	void on_key_pressed(GLFWwindow* window, int key, int, int action, int mods) {
		Handled_Key_Information key_info{};
		key_info.key = glfw_to_key(key);
		key_info.key_action = glfw_to_key_action(action);
		key_info.key_mods = glfw_to_key_mods(mods);
		handled_keys.push_back(key_info);
	}

	void on_char_callback(GLFWwindow* window, unsigned int code_point) {
		handled_chars.push_back((char32_t)code_point);
	}

	void init_key_handler(GLFWwindow* window) {
		glfwSetKeyCallback(window, on_key_pressed);
		glfwSetCharCallback(window, on_char_callback);
	}

	const std::vector<Handled_Key_Information>& get_keys_input_info() {
		return handled_keys;
	}

	void clean_keys_input() {
		handled_keys.clear();
	}

	const std::vector<char32_t>& get_chars_input() {
		return handled_chars;
	}

	void clean_chars_input() {
		handled_chars.clear();
	}

} // namespace Engine