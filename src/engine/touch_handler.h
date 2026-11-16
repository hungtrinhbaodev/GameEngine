#pragma once
#include <GLFW/glfw3.h>
#include <engine/structs.h>
#include <vector>

namespace Engine {

	void on_cursor_touch_position(GLFWwindow* window, double x, double y);

	void init_touch_handler(GLFWwindow* window);

	void handle_touches_logic();

	const std::vector<Handled_Touches_Information>& get_handled_touches();

} // namespace Engine