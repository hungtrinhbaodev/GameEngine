#pragma once
#include <GLFW/glfw3.h>
#include <engine/structs.h>

namespace Engine {

	void init_key_handler(GLFWwindow* window);

	const std::vector<Handled_Key_Information>& get_keys_input_info();

	void cleanup_keys_input();

	const std::vector<char32_t>& get_chars_input();

	void cleanup_chars_input();

} // namespace Engine