#include <engine/components.h>
#include <engine/edit_box.h>
#include <engine/engine.h>

namespace Engine {

	Handle create_edit_box() {
		auto& registry = get_registry();
		Handle edit_box = create_layout();
		Handle scroll_layout = create_scroll_layout();
		Handle inner_label = create_label();
		add_child(edit_box, scroll_layout);
		add_child_to_container(scroll_layout, inner_label);
		set_touch_enabled(scroll_layout, true);
		set_touch_caller(
			scroll_layout,
			[](Handle e, TOUCH_STATE touch_state, glm::vec2 touch_world_position, MOUSE_BUTTON touch_type, int mods) {

			}
		);
		set_key_input_enabled(edit_box, true);
		set_key_input_caller(edit_box, [](const Handle& e, KEY key, KEY_ACTION action, int mods, bool& swallow) {

		});
		set_char_input_enabled(edit_box, true);
		set_char_input_caller(edit_box, [](const Handle&, char32_t code_point, bool& swallows) {

		});
		Edit_Box_Information edit_box_info{};
		edit_box_info.scroll_layout = scroll_layout;
		edit_box_info.inner_label = inner_label;
		registry.add(edit_box.id(), Description{DESCRIPTION_TYPE::EDIT_BOX});
		registry.add(edit_box.id(), edit_box_info);
		return edit_box;
	}

} // namespace Engine