#include <cstdio>
#include <editor_internal.h>
#include <log.h>

namespace Editor {

	Handle make_panel(const Handle& parent, glm::vec2 position, glm::vec2 size, glm::vec3 color, float radius,
					  glm::vec2 anchor) {
		Handle layout = Engine::create_layout();
		Engine::set_content_size(layout, size);
		Engine::set_position(layout, position);
		Engine::set_anchor(layout, anchor);
		Engine::set_layout_background_enabled(layout, true);
		Engine::set_layout_background_color(layout, color);
		Engine::set_layout_border_radius(layout, radius);
		Engine::add_child(parent, layout);
		return layout;
	}

	Handle make_text(const Handle& parent, const std::string& text, int font_size, glm::vec3 color, glm::vec2 position,
					 glm::vec2 anchor) {
		Handle label = Engine::create_label();
		Engine::set_label_font_path(label, FONT_PATH);
		Engine::set_label_font_size(label, font_size);
		Engine::set_label_string(label, text);
		Engine::set_color(label, color);
		Engine::set_position(label, position);
		Engine::set_anchor(label, anchor);
		Engine::add_child(parent, label);
		return label;
	}

	// a label with a fixed box: long text ends with "..." instead of overflowing.
	Handle make_fitted_text(const Handle& parent, const std::string& text, int font_size, glm::vec3 color,
							glm::vec2 position, glm::vec2 size, Engine::LABEL_HORIZONTAL_ALIGN align) {
		Handle label = make_text(parent, text, font_size, color, position, {0.f, 0.f});
		Engine::set_content_size(label, size);
		Engine::set_label_render_fit_mode(label, Engine::LABEL_RENDER_FIT_ELLIPSIS);
		Engine::set_label_horizontal_align(label, align);
		Engine::set_label_vertical_align(label, Engine::LABEL_VERTICAL_ALIGN_CENTER);
		return label;
	}

	Handle make_button(const Handle& parent, const std::string& text, glm::vec2 position, glm::vec2 size,
					   glm::vec3 color, std::function<void()> on_click) {
		Handle button = make_panel(parent, position, size, color, 6.f);
		make_text(button, text, 14, TEXT, size * 0.5f, {0.5f, 0.5f});
		Engine::set_touch_enabled(button, true);
		Engine::set_touch_caller(button, [color](Handle e, Engine::TOUCH_STATE touch_state, glm::vec2, int, int) {
			bool down = touch_state == Engine::TOUCH_STATE::TOUCH_BEGAN || touch_state == Engine::TOUCH_STATE::TOUCH_MOVE;
			Engine::set_layout_background_color(e, down ? color * 0.75f : color);
		});
		Engine::set_click_caller(button, [on_click](Handle, glm::vec2, int, int) { on_click(); });
		return button;
	}

	void set_status(const std::string& text) {
		Log::info("[editor]", text);
		if (Engine::is_valid(state.status))
			Engine::set_label_string(state.status, text);
	}

	std::string vec_text(glm::vec2 v) {
		char buffer[64];
		std::snprintf(buffer, sizeof(buffer), "%.0f, %.0f", v.x, v.y);
		return buffer;
	}

	std::string number_text(float value, int decimals) {
		char buffer[32];
		std::snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);
		return buffer;
	}

	glm::vec2 center_of(const Handle& e) {
		auto rect = Engine::get_world_rect(e);
		return (rect[0] + rect[1] + rect[2] + rect[3]) * 0.25f;
	}

} // namespace Editor
