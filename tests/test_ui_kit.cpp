#include <cmath>
#include <cstdio>
#include <engine/components.h>
#include <log.h>
#include <test_ui.h>

namespace Test {

	Scene_State scene{};

	struct Press_State {
		glm::vec3 base_color{1.f};
		glm::vec2 start{0.f};
		bool pressed = false;
	};

	Scene_State& get_scene() {
		return scene;
	}

	Handle track(const Handle& e) {
		scene.nodes.push_back(e);
		return e;
	}

	std::vector<Handle> collect_subtree(const Handle& e) {
		Registry& registry = Engine::get_registry();
		std::vector<Handle> result{};
		std::vector<Handle> stack{e};
		while (!stack.empty()) {
			Handle node = stack.back();
			stack.pop_back();
			result.push_back(node);
			if (!registry.has<Engine::Tree_Information>(node.id()))
				continue;
			const auto& [tree_info] = registry.read<Engine::Tree_Information>(node.id());
			for (const auto& child : tree_info.children) {
				stack.push_back(child);
			}
		}
		return result;
	}

	void remove_handles_from(std::vector<Handle>& list, const std::vector<Handle>& removed) {
		for (int i = 0; i < (int)list.size(); i++) {
			for (const auto& r : removed) {
				if (same(list[i], r)) {
					list[i] = list.back();
					list.pop_back();
					i--;
					break;
				}
			}
		}
	}

	void destroy(const Handle& e) {
		if (e.is_invalid())
			return;
		auto removed = collect_subtree(e);
		Engine::clean_up(e);
		remove_handles_from(scene.nodes, removed);
		remove_handles_from(scene.detached, removed);
		if (same(scene.root, e)) {
			scene.root = Handle::make_invalid_handle();
		}
	}

	void clear_scene() {
		auto detached = scene.detached;
		for (const auto& e : detached) {
			destroy(e);
		}
		if (!scene.root.is_invalid()) {
			destroy(scene.root);
		}
		scene = Scene_State{};
	}

	Handle make_page(const std::string& name, const glm::vec3& color) {
		clear_scene();
		scene.name = name;
		Handle page = Engine::create_layout();
		Engine::set_size_pinning(page, {0.f, 0.f, 0.f, 0.f});
		Engine::set_layout_background_enabled(page, true);
		Engine::set_layout_background_color(page, color);
		Engine::add_child(Engine::get_root(), page);
		scene.root = page;
		return track(page);
	}

	Handle make_layout(
		const Handle& parent, const glm::vec2& position, const glm::vec2& size, const glm::vec3& color,
		float border_radius, const glm::vec2& anchor
	) {
		Handle layout = Engine::create_layout();
		Engine::set_content_size(layout, size);
		Engine::set_position(layout, position);
		Engine::set_anchor(layout, anchor);
		Engine::set_layout_background_enabled(layout, true);
		Engine::set_layout_background_color(layout, color);
		Engine::set_layout_border_radius(layout, border_radius);
		Engine::add_child(parent, layout);
		return track(layout);
	}

	Handle make_label(
		const Handle& parent, const std::string& text, int font_size, const glm::vec3& color, const glm::vec2& position,
		const glm::vec2& anchor
	) {
		Handle label = Engine::create_label();
		Engine::set_label_font_path(label, FONT_PATH);
		Engine::set_label_font_size(label, font_size);
		Engine::set_label_string(label, text);
		Engine::set_color(label, color);
		Engine::set_position(label, position);
		Engine::set_anchor(label, anchor);
		Engine::add_child(parent, label);
		return track(label);
	}

	Handle make_image(
		const Handle& parent, const std::string& path, const glm::vec2& position, const glm::vec2& anchor
	) {
		Handle image = Engine::create_image();
		Engine::set_image_path(image, path);
		Engine::set_position(image, position);
		Engine::set_anchor(image, anchor);
		Engine::add_child(parent, image);
		return track(image);
	}

	void set_touch_action(const Handle& e, const Touch_Action& action) {
		Engine::set_touch_enabled(e, true);
		Engine::set_touch_caller(e, [action](Handle h, Engine::TOUCH_STATE touch_state, glm::vec2 world_position) {
			try {
				action(h, touch_state, world_position);
			} catch (const std::exception& exception) {
				Log::info("[test] exception inside touch callback:", exception.what());
			}
		});
	}

	Handle make_button(
		const Handle& parent, const std::string& text, const glm::vec2& position, const glm::vec2& size,
		const glm::vec3& color, const std::function<void()>& on_click, const glm::vec2& anchor
	) {
		Handle button = make_layout(parent, position, size, color, 8.f, anchor);
		make_label(button, text, 16, {1.f, 1.f, 1.f}, size * 0.5f);
		auto state = std::make_shared<Press_State>();
		state->base_color = color;
		set_touch_action(
			button, [state, on_click](Handle e, Engine::TOUCH_STATE touch_state, glm::vec2 world_position) {
				switch (touch_state) {
					case Engine::TOUCH_STATE::TOUCH_BEGAN: {
						state->pressed = true;
						state->start = world_position;
						Engine::set_layout_background_color(e, state->base_color * 0.7f);
						break;
					}
					case Engine::TOUCH_STATE::TOUCH_ENDED: {
						if (!state->pressed)
							break;
						state->pressed = false;
						Engine::set_layout_background_color(e, state->base_color);
						/**
						 * @Note: on_click may destroy this button, so it must be the last call.
						 */
						if (glm::distance(state->start, world_position) < CLICK_SLOP && on_click) {
							on_click();
						}
						break;
					}
					default: {
						break;
					}
				}
			}
		);
		return button;
	}

	void update_slider_view(const std::shared_ptr<Slider_State>& slider) {
		Engine::set_position(slider->knob, {slider->value * slider->width, 3.f});
		int value_255 = (int)std::round(slider->value * 255.f);
		Engine::set_label_string(slider->label, slider->name + " " + std::to_string(value_255));
	}

	std::shared_ptr<Slider_State> make_slider(
		const Handle& parent, const std::string& name, const glm::vec2& position, float width, float value,
		const std::function<void(float)>& on_change, const std::function<void(float)>& on_release
	) {
		auto slider = std::make_shared<Slider_State>();
		slider->name = name;
		slider->width = width;
		slider->value = glm::clamp(value, 0.f, 1.f);
		slider->on_change = on_change;
		slider->on_release = on_release;
		slider->track = make_layout(parent, position, {width, 6.f}, {0.35f, 0.37f, 0.42f}, 3.f);
		slider->knob = make_layout(slider->track, {0.f, 3.f}, {14.f, 22.f}, {0.92f, 0.93f, 0.95f}, 5.f, {0.5f, 0.5f});
		slider->label = make_label(slider->track, "", 14, {0.85f, 0.87f, 0.9f}, {0.f, 20.f}, {0.f, 0.5f});
		update_slider_view(slider);
		std::weak_ptr<Slider_State> weak = slider;
		set_touch_action(slider->knob, [weak](Handle, Engine::TOUCH_STATE touch_state, glm::vec2 world_position) {
			auto slider = weak.lock();
			if (!slider)
				return;
			switch (touch_state) {
				case Engine::TOUCH_STATE::TOUCH_BEGAN: {
					slider->start_x = world_position.x;
					slider->start_value = slider->value;
					break;
				}
				case Engine::TOUCH_STATE::TOUCH_MOVE: {
					float delta = (world_position.x - slider->start_x) / std::max(slider->width, 1.f);
					slider->value = glm::clamp(slider->start_value + delta, 0.f, 1.f);
					update_slider_view(slider);
					if (slider->on_change)
						slider->on_change(slider->value);
					break;
				}
				case Engine::TOUCH_STATE::TOUCH_ENDED: {
					if (slider->on_release)
						slider->on_release(slider->value);
					break;
				}
				default: {
					break;
				}
			}
		});
		return slider;
	}

	void set_slider_value(const std::shared_ptr<Slider_State>& slider, float value) {
		slider->value = glm::clamp(value, 0.f, 1.f);
		update_slider_view(slider);
	}

	glm::vec3 hsv_to_rgb(float h, float s, float v) {
		h = std::fmod(std::max(h, 0.f), 1.f) * 6.f;
		int i = (int)std::floor(h);
		float f = h - (float)i;
		float p = v * (1.f - s);
		float q = v * (1.f - s * f);
		float t = v * (1.f - s * (1.f - f));
		switch (i % 6) {
			case 0:
				return {v, t, p};
			case 1:
				return {q, v, p};
			case 2:
				return {p, v, t};
			case 3:
				return {p, q, v};
			case 4:
				return {t, p, v};
			default:
				return {v, p, q};
		}
	}

	std::string to_hex(const glm::vec3& color) {
		char buffer[8];
		auto to_byte = [](float value) { return (int)std::round(glm::clamp(value, 0.f, 1.f) * 255.f); };
		std::snprintf(buffer, sizeof(buffer), "#%02X%02X%02X", to_byte(color.r), to_byte(color.g), to_byte(color.b));
		return buffer;
	}

} // namespace Test
