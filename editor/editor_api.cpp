#include <editor_api.h>
#include <engine/components.h>
#include <files.h>
#include <math_custom.h>
#include <utils.h>
#include <vulkan/vk_draw.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Editor_Api {

	namespace {
		GLFWwindow* editor_window = nullptr;
	}

	void init(GLFWwindow* window) {
		editor_window = window;
	}

	glm::vec2 get_texture_size(const std::string& image_path) {
		if (!Files::is_file_exists(Utils::get_root_path() + image_path))
			return {0.f, 0.f};
		uint32_t texture_id = Vulkan::load_texture(image_path);
		const auto& view = Vulkan::view_texture(texture_id);
		return {view.width, view.height};
	}

	glm::vec2 get_cursor_world_position() {
		if (editor_window == nullptr)
			return {0.f, 0.f};
		double x = 0.0, y = 0.0;
		glfwGetCursorPos(editor_window, &x, &y);
		glm::vec2 window = Engine::get_window_size();
		return {(float)x, window.y - (float)y};
	}

	glm::vec2 to_child_space(const Handle& e, const glm::vec2& world_position) {
		return Engine::to_my_position_space(e, world_position) + Engine::get_anchor(e) * Engine::get_content_size(e);
	}

	void set_scroll_drag_enabled(const Handle& scroll_layout, bool enabled) {
		Engine::set_layout_scroll_touch_enabled(scroll_layout, enabled);
		Registry& registry = Engine::get_registry();
		for (const auto& child : Engine::get_children(scroll_layout)) {
			if (!registry.has<Engine::Description>(child.id()))
				continue;
			const auto& [description] = registry.read<Engine::Description>(child.id());
			if (description.value == Engine::DESCRIPTION_TYPE::SCROLL_LISTENER_CONTAINER)
				Engine::set_touch_enabled(child, enabled);
		}
	}

	bool is_point_in_node(const Handle& e, const glm::vec2& world_position) {
		if (!Engine::is_valid(e))
			return false;
		return Math::point_in_rect(Engine::get_world_rect(e), world_position);
	}

} // namespace Editor_Api
