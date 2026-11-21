#include <engine/components.h>
#include <engine/edit_box.h>
#include <engine/engine.h>

namespace Engine {

	void enter_focus_edit_box(const Handle& e) {
		// TODO: add border to bounding box.
		set_layout_border_width(e, 2.f);
	}

	void exit_focus_edit_box(const Handle& e) {
		// TODO: release border to bounding box.
		set_layout_border_width(e, 0.f);
	}

	void update_caret_by_world_position(const Handle& e, glm::vec2 touch_world_position) {
		auto& registry = get_registry();
		auto [edit_box_info] = registry.get<Edit_Box_Information>(e.id());
		glm::vec2 label_size = get_content_size(edit_box_info.inner_label);
		int char_index = find_label_character_index(
			edit_box_info.inner_label, touch_world_position, glm::distance(glm::vec2{0.f}, label_size * 0.5f)
		);
		edit_box_info.caret_location = std::max(0, char_index);
	}

	void handle_edit_box_touched(
		Handle e, TOUCH_STATE touch_state, glm::vec2 touch_world_position, MOUSE_BUTTON touch_type, int mods
	) {
		auto& registry = get_registry();
		auto [edit_box_info] = registry.get<Edit_Box_Information>(e.id());
		switch (touch_state) {
			case TOUCH_STATE::TOUCH_BEGAN: {

				break;
			}
			case TOUCH_STATE::TOUCH_MOVE: {
				break;
			}
			case TOUCH_STATE::TOUCH_ENDED: {
				switch (edit_box_info.edit_box_state) {
					case EDIT_BOX_STATE::EDIT_BOX_STATE_IDLE: {
						/**
						 * Set focus to change style of edit box
						 * and take controll the key input!
						 */
						set_key_focus(e);
						break;
					}
					default: {
						break;
					}
				}
				update_caret_by_world_position(e, touch_world_position);
				break;
			}
			default: {
				break;
			}
		}
	}

	Handle create_edit_box() {
		auto& registry = get_registry();
		Handle edit_box = create_layout();
		Handle scroll_layout = create_scroll_layout();
		Handle inner_label = create_label();
		Handle caret_handle = create_layout();

		add_child(edit_box, scroll_layout);
		add_child_to_container(scroll_layout, inner_label);
		add_child_to_container(scroll_layout, caret_handle);

		set_content_size(caret_handle, {2.f, 0.f});
		set_layout_background_enabled(caret_handle, true);
		set_layout_background_color(caret_handle, {0.f, 0.f, 0.f});
		set_touch_enabled(scroll_layout, true);
		set_layout_scroll_touch_enabled(scroll_layout, false);
		set_touch_caller(
			scroll_layout,
			[](Handle e, TOUCH_STATE touch_state, glm::vec2 touch_world_position, MOUSE_BUTTON touch_type, int mods) {
				handle_edit_box_touched(e, touch_state, touch_world_position, touch_type, mods);
			}
		);
		set_key_input_enabled(edit_box, true);
		set_key_input_caller(edit_box, [](const Handle& e, KEY key, KEY_ACTION action, int mods, bool& swallow) {

		});
		set_focus_enter_caller(edit_box, [](const Handle& e) { enter_focus_edit_box(e); });
		set_focus_exit_caller(edit_box, [](const Handle& e) { exit_focus_edit_box(e); });
		set_char_input_enabled(edit_box, true);
		set_char_input_caller(edit_box, [](const Handle&, char32_t code_point, bool& swallows) {

		});
		set_layout_border_color(edit_box, {0.f, 0.f, 0.f});
		Edit_Box_Information edit_box_info{};
		edit_box_info.scroll_layout = scroll_layout;
		edit_box_info.inner_label = inner_label;
		registry.add(edit_box.id(), Description{DESCRIPTION_TYPE::EDIT_BOX});
		registry.add(edit_box.id(), edit_box_info);
		return edit_box;
	}

} // namespace Engine