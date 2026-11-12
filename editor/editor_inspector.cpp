#include <algorithm>
#include <editor_internal.h>

namespace Editor {

	namespace {
		struct Property {
			std::string name;
			float step;	 // value change per pixel dragged
			float min_value;
			float max_value;
			int decimals;
		};

		const std::vector<Property> COMMON = {
			{"X", 1.f, -10000.f, 10000.f, 0},		  {"Y", 1.f, -10000.f, 10000.f, 0},
			{"Rotation", 0.5f, -3600.f, 3600.f, 1}, {"Scale", 0.01f, 0.05f, 20.f, 2},
			{"Opacity", 0.005f, 0.f, 1.f, 2},		  {"Z", 0.2f, 0.f, 1e7f, 0},
		};
		const std::vector<Property> LAYOUT = {
			{"Width", 1.f, 1.f, 10000.f, 0},
			{"Height", 1.f, 1.f, 10000.f, 0},
			{"Radius", 0.25f, 0.f, 500.f, 0},
			{"Border", 0.1f, 0.f, 50.f, 1},
		};
		const std::vector<Property> LABEL = {{"Font size", 0.2f, 6.f, 72.f, 0}};

		const std::vector<glm::vec3> SWATCHES = {
			{1.f, 1.f, 1.f},	{0.13f, 0.15f, 0.19f}, {0.86f, 0.3f, 0.3f}, {0.95f, 0.6f, 0.2f},
			{0.95f, 0.85f, 0.3f}, {0.35f, 0.75f, 0.4f},  {0.25f, 0.5f, 0.95f}, {0.6f, 0.4f, 0.85f},
		};

		const Property* find_property(const std::string& name) {
			for (const auto* list : {&COMMON, &LAYOUT, &LABEL}) {
				for (const auto& property : *list) {
					if (property.name == name)
						return &property;
				}
			}
			return nullptr;
		}

		void set_property_value(const Handle& node, const std::string& property, float value) {
			glm::vec2 position = Engine::get_position(node);
			glm::vec2 size = Engine::get_content_size(node);
			if (property == "X")
				Engine::set_position(node, {value, position.y});
			else if (property == "Y")
				Engine::set_position(node, {position.x, value});
			else if (property == "Rotation")
				Engine::set_rotation(node, value);
			else if (property == "Scale")
				Engine::set_scale(node, {value, value});
			else if (property == "Opacity")
				Engine::set_opacity(node, value);
			else if (property == "Z")
				Engine::set_z_order(node, (uint32_t)std::max(0.f, value));
			else if (property == "Width")
				Engine::set_content_size(node, {value, size.y});
			else if (property == "Height")
				Engine::set_content_size(node, {size.x, value});
			else if (property == "Radius")
				Engine::set_layout_border_radius(node, value);
			else if (property == "Border")
				Engine::set_layout_border_width(node, value);
			else if (property == "Font size")
				Engine::set_label_font_size(node, (int)std::round(value));
		}

		// one draggable number field: press, drag left / right, release = one undo step.
		void add_field(const Handle& parent, const Property& property, float y) {
			const float label_width = 96.f;
			make_text(parent, property.name, 13, MUTED, {14.f, y + 12.f});
			Field field;
			field.property = property.name;
			field.box = make_panel(parent, {14.f + label_width, y}, {RIGHT_WIDTH - 28.f - label_width, 24.f}, FIELD_BG, 4.f);
			field.value = make_text(field.box, "", 13, TEXT, {(RIGHT_WIDTH - 28.f - label_width) * 0.5f, 12.f}, {0.5f, 0.5f});
			std::string name = property.name;
			Engine::set_touch_enabled(field.box, true);
			Engine::set_touch_cancel_on_fast_move(field.box, false);
			Engine::set_touch_caller(field.box, [name](Handle box, Engine::TOUCH_STATE touch_state, glm::vec2 world, int, int mods) {
				const Property* property = find_property(name);
				if (property == nullptr || !Engine::is_valid(state.selected))
					return;
				switch (touch_state) {
					case Engine::TOUCH_STATE::TOUCH_BEGAN: {
						state.drag_start = world;
						state.scrub_start_value = get_property_value(state.selected, name);
						Engine::claim_touch(box);
						Engine::set_layout_background_color(box, ROW_SELECTED);
						break;
					}
					case Engine::TOUCH_STATE::TOUCH_MOVE: {
						// Shift = 10x finer, like Godot's slow drag.
						float step = property->step * ((mods & GLFW_MOD_SHIFT) ? 0.1f : 1.f);
						float value = state.scrub_start_value + (world.x - state.drag_start.x) * step;
						value = std::clamp(value, property->min_value, property->max_value);
						set_property_value(state.selected, name, value);
						update_frame();
						refresh_inspector_values();
						break;
					}
					default: {
						Engine::set_layout_background_color(box, FIELD_BG);
						if (Engine::is_valid(state.selected))
							commit(Engine::get_name(state.selected) + " " + name + " = " +
								   number_text(get_property_value(state.selected, name), property->decimals));
						break;
					}
				}
			});
			state.fields.push_back(field);
		}
	} // namespace

	float get_property_value(const Handle& node, const std::string& property) {
		if (property == "X")
			return Engine::get_position(node).x;
		if (property == "Y")
			return Engine::get_position(node).y;
		if (property == "Rotation")
			return Engine::get_rotation(node);
		if (property == "Scale")
			return Engine::get_scale(node).x;
		if (property == "Opacity")
			return Engine::get_opacity(node);
		if (property == "Z")
			return (float)Engine::get_z_order(node);
		if (property == "Width")
			return Engine::get_content_size(node).x;
		if (property == "Height")
			return Engine::get_content_size(node).y;
		if (property == "Radius")
			return Engine::get_layout_border_radius(node);
		if (property == "Border")
			return Engine::get_layout_border_width(node);
		if (property == "Font size")
			return (float)Engine::get_label_font_size(node);
		return 0.f;
	}

	void build_inspector_panel(const Handle& parent, glm::vec2 position, glm::vec2 size) {
		state.inspector = make_panel(parent, position, size, PANEL_BG, 0.f);
		make_text(state.inspector, "Inspector", 16, TEXT, {14.f, size.y - 18.f});
		make_text(state.inspector, "drag a number left / right (Shift = fine)", 11, MUTED, {14.f, size.y - 36.f});
	}

	void refresh_inspector_values() {
		if (!Engine::is_valid(state.selected))
			return;
		for (const auto& field : state.fields) {
			const Property* property = find_property(field.property);
			if (property != nullptr && Engine::is_valid(field.value))
				Engine::set_label_string(field.value, number_text(get_property_value(state.selected, field.property), property->decimals));
		}
	}

	void rebuild_inspector() {
		if (!Engine::is_valid(state.inspector))
			return;
		if (Engine::is_valid(state.inspector_content))
			Engine::clean_up(state.inspector_content);
		state.fields.clear();
		glm::vec2 size = Engine::get_content_size(state.inspector);
		state.inspector_content = Engine::create_layout();
		Engine::set_content_size(state.inspector_content, {size.x, size.y - 48.f});
		Engine::add_child(state.inspector, state.inspector_content);
		const Handle& content = state.inspector_content;
		float y = size.y - 48.f;
		auto next = [&y](float height) { return y -= height; };

		if (!Engine::is_valid(state.selected)) {
			make_text(content, "Nothing selected", 13, MUTED, {14.f, next(24.f)});
			make_text(content, "+ Layout / + Label in the toolbar,", 12, MUTED, {14.f, next(22.f)});
			make_text(content, "or drag an image from FileSystem.", 12, MUTED, {14.f, next(18.f)});
			return;
		}
		const Handle node = state.selected;
		const Kind kind = kind_of(node);
		make_fitted_text(content, Engine::get_name(node), 15, TEXT, {14.f, next(28.f)}, {size.x - 28.f, 22.f},
						 Engine::LABEL_HORIZONTAL_ALIGN_LEFT);
		std::string detail = Scene_Io::kind_name(kind);
		if (kind == Kind::IMAGE)
			detail += "  " + Engine::get_image_path(node);
		if (kind == Kind::LABEL)
			detail += "  \"" + Engine::get_label_string(node) + "\"";
		make_fitted_text(content, detail, 12, MUTED, {14.f, next(20.f)}, {size.x - 28.f, 18.f},
						 Engine::LABEL_HORIZONTAL_ALIGN_LEFT);
		next(6.f);
		for (const auto& property : COMMON)
			add_field(content, property, next(28.f));
		if (kind == Kind::LAYOUT) {
			for (const auto& property : LAYOUT)
				add_field(content, property, next(28.f));
		}
		if (kind == Kind::LABEL) {
			for (const auto& property : LABEL)
				add_field(content, property, next(28.f));
		}

		// colour: layout fill, or the label / image tint.
		next(8.f);
		make_text(content, kind == Kind::LAYOUT ? "Fill" : "Color", 13, MUTED, {14.f, next(22.f) + 11.f});
		float x = 110.f;
		for (const auto& color : SWATCHES) {
			Handle swatch = make_panel(content, {x, y}, {20.f, 20.f}, color, 4.f);
			x += 22.f;
			Engine::set_touch_enabled(swatch, true);
			Engine::set_click_caller(swatch, [color](Handle, glm::vec2, int, int) {
				if (!Engine::is_valid(state.selected))
					return;
				if (kind_of(state.selected) == Kind::LAYOUT)
					Engine::set_layout_background_color(state.selected, color);
				else
					Engine::set_color(state.selected, color);
				commit("Color of " + Engine::get_name(state.selected));
			});
		}

		// toggles + actions
		next(12.f);
		const glm::vec2 half{(size.x - 36.f) / 2.f, 28.f};
		float row_y = next(32.f);
		make_button(content, Engine::is_visible(node) ? "Visible: on" : "Visible: off", {14.f, row_y}, half, BUTTON, [] {
			if (!Engine::is_valid(state.selected))
				return;
			Engine::set_visible(state.selected, !Engine::is_visible(state.selected));
			commit(Engine::get_name(state.selected) + (Engine::is_visible(state.selected) ? " shown" : " hidden"));
			rebuild_inspector();
			rebuild_tree();
		});
		if (kind == Kind::LAYOUT) {
			make_button(content, Engine::is_layout_clipping_enabled(node) ? "Clip: on" : "Clip: off",
						{22.f + half.x, row_y}, half, BUTTON, [] {
							if (!Engine::is_valid(state.selected))
								return;
							Engine::set_layout_clipping_enabled(state.selected, !Engine::is_layout_clipping_enabled(state.selected));
							commit(Engine::get_name(state.selected) + " clipping toggled");
							rebuild_inspector();
						});
		}
		row_y = next(36.f);
		make_button(content, "Delete", {14.f, row_y}, half, DANGER, [] { delete_selected(); });
		make_button(content, "Duplicate", {22.f + half.x, row_y}, half, BUTTON, [] { duplicate_selected(); });
		row_y = next(36.f);
		make_button(content, "To front", {14.f, row_y}, half, BUTTON, [] {
			if (!Engine::is_valid(state.selected))
				return;
			Engine::set_z_order(state.selected, state.next_z++);
			refresh_inspector_values();
			commit(Engine::get_name(state.selected) + " to front");
		});
		make_button(content, "To back", {22.f + half.x, row_y}, half, BUTTON, [] {
			if (!Engine::is_valid(state.selected) || state.back_z == 0)
				return;
			Engine::set_z_order(state.selected, state.back_z--);
			refresh_inspector_values();
			commit(Engine::get_name(state.selected) + " to back");
		});
		refresh_inspector_values();
	}

} // namespace Editor
