#pragma once

#include <GLFW/glfw3.h>
#include <engine/structs.h>

namespace Engine {

	void on_cursor_scroll_position(GLFWwindow* window, double x, double y);

	void init_scroll_handler(GLFWwindow* window);

	bool has_scrolling();

	glm::vec2 get_world_scrolling_position();

	float get_scroll_distance();

} // namespace Engine