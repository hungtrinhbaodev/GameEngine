#include <engine/engine.h>
#include <engine/scroll_handler.h>

namespace Engine {

	glm::vec2 world_scroll_position{0.f, 0.f};

	bool is_scrolling = false;

	float scroll_distance = 0.f;

	void on_scroll_fire(GLFWwindow* window, double xoffset, double yoffset) {
		is_scrolling = yoffset > 0;
		scroll_distance = yoffset;
	}

	void on_cursor_scroll_position(GLFWwindow* window, double x, double y) {
		glm::vec2 screen_size = get_window_size();
		world_scroll_position = glm::vec2{x, screen_size.y - y};
	}

	void init_scroll_handler(GLFWwindow* window) {
		glfwSetScrollCallback(window, on_scroll_fire);
	}

	bool has_scrolling() {
		return is_scrolling;
	}

	glm::vec2 get_world_scrolling_position() {
		return world_scroll_position;
	}

	float get_scroll_distance() {
		return scroll_distance;
	}

} // namespace Engine