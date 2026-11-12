#include <engine/components.h>
#include <log.h>
#include <test_ui.h>

namespace Test {

	namespace {
		const glm::vec3 INK{0.13f, 0.15f, 0.19f};
		const glm::vec3 MUTED{0.42f, 0.45f, 0.5f};
		const glm::vec3 WHITE{1.f, 1.f, 1.f};
		const glm::vec3 BLUE{0.22f, 0.45f, 0.85f};
		const glm::vec3 RED{0.82f, 0.27f, 0.27f};
		const glm::vec3 GREY{0.55f, 0.58f, 0.62f};
		const glm::vec3 CARD{1.f, 1.f, 1.f};
	} // namespace

	// ============================== 1. MODAL ==============================

	namespace Modal {
		Handle page = Handle::make_invalid_handle();
		Handle status = Handle::make_invalid_handle();
		Handle modal_root = Handle::make_invalid_handle();
		bool destroy_on_close = false;
		int confirm_count = 0;
		int nine_scale_count = 0;

		void set_status(const std::string& text) {
			Engine::set_label_string(status, "Status: " + text);
		}

		void close_modal() {
			if (modal_root.is_invalid())
				return;
			if (destroy_on_close) {
				destroy(modal_root);
				modal_root = Handle::make_invalid_handle();
			} else {
				Engine::set_visible(modal_root, false);
			}
		}

		void build_modal() {
			glm::vec2 window = Engine::get_window_size();
			/**
			 * @Note: overlay = full screen layout, half transparent, swallows touches.
			 * Dialog is a child, but opacity does not cascade so it stays solid.
			 */
			modal_root = Engine::create_layout();
			Engine::set_size_pinning(modal_root, {0.f, 0.f, 0.f, 0.f});
			Engine::set_layout_background_enabled(modal_root, true);
			Engine::set_layout_background_color(modal_root, {0.f, 0.f, 0.f});
			Engine::set_opacity(modal_root, 0.55f);
			Engine::set_z_order(modal_root, 100);
			Engine::add_child(page, modal_root);
			track(modal_root);
			set_touch_action(modal_root, [](Handle e, Engine::TOUCH_STATE touch_state, glm::vec2) {
				if (touch_state != Engine::TOUCH_STATE::TOUCH_ENDED)
					return;
				/**
				 * @Note: if this logs visible=0 the hidden overlay still eats touches.
				 */
				Log::info("[modal] overlay touched, visible =", Engine::is_visible(e));
				close_modal();
			});

			glm::vec2 size{460.f, 260.f};
			Handle dialog = make_layout(modal_root, window * 0.5f, size, CARD, 14.f, {0.5f, 0.5f});
			Engine::set_opacity(dialog, 1.f);
			Engine::set_layout_border_width(dialog, 2.f);
			Engine::set_layout_border_color(dialog, {0.8f, 0.82f, 0.86f});
			// swallow touches on the dialog so clicks inside do not close it.
			Engine::set_touch_enabled(dialog, true);

			make_label(dialog, "Delete 3 files?", 22, INK, {24.f, size.y - 36.f}, {0.f, 0.5f});
			make_label(
				dialog, "This cannot be undone.\nThe files move to the bin.", 16, MUTED, {24.f, size.y - 100.f},
				{0.f, 0.5f}
			);
			make_button(dialog, "x", {size.x - 44.f, size.y - 44.f}, {32.f, 32.f}, GREY, []() {
				set_status("closed with x");
				close_modal();
			});
			make_button(dialog, "Destroy self", {24.f, 20.f}, {140.f, 44.f}, GREY, []() {
				// clean_up the whole modal from inside its own touch callback.
				set_status("modal destroyed itself inside its callback");
				destroy(modal_root);
				modal_root = Handle::make_invalid_handle();
			});
			make_button(dialog, "Cancel", {size.x - 254.f, 20.f}, {110.f, 44.f}, GREY, []() {
				set_status("cancelled");
				close_modal();
			});
			make_button(dialog, "Confirm", {size.x - 134.f, 20.f}, {110.f, 44.f}, RED, []() {
				confirm_count++;
				set_status("confirmed " + std::to_string(confirm_count) + " time(s)");
				close_modal();
			});
		}

		void open_modal(bool destroy_mode) {
			if (!modal_root.is_invalid() && destroy_on_close != destroy_mode) {
				destroy(modal_root);
				modal_root = Handle::make_invalid_handle();
			}
			destroy_on_close = destroy_mode;
			if (modal_root.is_invalid()) {
				build_modal();
			}
			Engine::set_visible(modal_root, true);
		}
	} // namespace Modal

	void build_modal_scene() {
		using namespace Modal;
		glm::vec2 window = Engine::get_window_size();
		page = make_page("modal", {0.94f, 0.95f, 0.96f});
		modal_root = Handle::make_invalid_handle();
		confirm_count = 0;
		nine_scale_count = 0;

		Handle header = make_layout(page, {0.f, 0.f}, {window.x, 64.f}, {0.16f, 0.2f, 0.28f});
		Engine::set_size_pinning(header, {0.f, 0.f, 0.f, window.y - 64.f});
		make_label(header, "UI test 1: website modal", 22, WHITE, {24.f, 32.f}, {0.f, 0.5f});

		status = make_label(page, "Status: idle", 18, INK, {24.f, window.y - 96.f}, {0.f, 0.5f});

		glm::vec2 card_size{340.f, 220.f};
		float top = window.y - 130.f;

		// card A: open by visibility.
		Handle card_a = make_layout(page, {24.f, top}, card_size, CARD, 10.f, {0.f, 1.f});
		make_label(card_a, "Visible mode", 20, INK, {20.f, card_size.y - 30.f}, {0.f, 0.5f});
		make_label(
			card_a, "Close hides the modal with\nset_visible(false). Click where\nit was: page must still respond.", 14,
			MUTED, {20.f, card_size.y - 100.f}, {0.f, 0.5f}
		);
		make_button(card_a, "Open dialog", {20.f, 20.f}, {160.f, 44.f}, BLUE, []() {
			open_modal(false);
			set_status("opened (visible mode)");
		});

		// card B: open by create/destroy.
		Handle card_b = make_layout(page, {24.f + card_size.x + 20.f, top}, card_size, CARD, 10.f, {0.f, 1.f});
		make_label(card_b, "Create / destroy mode", 20, INK, {20.f, card_size.y - 30.f}, {0.f, 0.5f});
		make_label(
			card_b, "Every open builds a new modal,\nevery close calls clean_up.\nPress P to check pools.", 14, MUTED,
			{20.f, card_size.y - 100.f}, {0.f, 0.5f}
		);
		make_button(card_b, "Build dialog", {20.f, 20.f}, {160.f, 44.f}, BLUE, []() {
			open_modal(true);
			set_status("opened (destroy mode)");
		});

		// card C: image + nine scale button.
		Handle card_c = make_layout(page, {24.f + 2.f * (card_size.x + 20.f), top}, card_size, CARD, 10.f, {0.f, 1.f});
		make_label(card_c, "Image + nine-scale", 20, INK, {20.f, card_size.y - 30.f}, {0.f, 0.5f});
		Handle icon = make_image(card_c, "res/AddonIcon5.png", {card_size.x - 70.f, card_size.y - 80.f});
		Engine::set_scale(icon, {0.6f, 0.6f});
		Handle nine = make_image(card_c, "res/BtnGreen.png", {20.f, 20.f}, {0.f, 0.f});
		Engine::set_nine_scale_enabled(nine, true);
		Engine::set_nine_scale_rect_ratio(nine, {0.25f, 0.3f, 0.5f, 0.4f});
		Engine::set_content_size(nine, {220.f, 56.f});
		Engine::set_image_nine_scale_size(nine, {220.f, 56.f});
		make_label(nine, "Nine-scale button", 16, WHITE, {110.f, 28.f});
		set_touch_action(nine, [](Handle, Engine::TOUCH_STATE touch_state, glm::vec2) {
			if (touch_state != Engine::TOUCH_STATE::TOUCH_ENDED)
				return;
			nine_scale_count++;
			set_status("nine-scale image clicked " + std::to_string(nine_scale_count) + " time(s)");
		});

		Log::info("[test] modal scene built, nodes:", get_scene().nodes.size());
	}

	// ============================== 2. COLOR PICKER ==============================

	namespace Picker {
		const int COLUMNS = 12;
		const int ROWS = 6;
		const float CELL = 30.f;
		const float GAP = 4.f;
		const int MAX_RECENT = 24;

		Handle preview = Handle::make_invalid_handle();
		Handle preview_text = Handle::make_invalid_handle();
		Handle hex_label = Handle::make_invalid_handle();
		Handle rgb_label = Handle::make_invalid_handle();
		Handle selected_swatch = Handle::make_invalid_handle();
		Handle recent_scroll = Handle::make_invalid_handle();
		std::vector<Handle> recent_items{};
		std::shared_ptr<Slider_State> sliders[3];
		glm::vec3 current{0.22f, 0.45f, 0.85f};

		void refresh_preview() {
			Engine::set_layout_background_color(preview, current);
			float luminance = 0.299f * current.r + 0.587f * current.g + 0.114f * current.b;
			Engine::set_color(preview_text, luminance > 0.55f ? INK : WHITE);
			Engine::set_label_string(hex_label, to_hex(current));
			Engine::set_label_string(
				rgb_label, "rgb(" + std::to_string((int)std::round(current.r * 255.f)) + ", " +
							   std::to_string((int)std::round(current.g * 255.f)) + ", " +
							   std::to_string((int)std::round(current.b * 255.f)) + ")"
			);
		}

		void refresh_sliders() {
			for (int i = 0; i < 3; i++) {
				set_slider_value(sliders[i], current[i]);
			}
		}

		void layout_recent() {
			const float step = 48.f;
			for (int i = 0; i < (int)recent_items.size(); i++) {
				Engine::set_position(recent_items[i], {8.f + step * (float)i, 8.f});
			}
			float width = 8.f + step * (float)recent_items.size();
			Engine::set_inner_container_size(recent_scroll, {std::max(width, 592.f), 56.f});
		}

		void push_recent(const glm::vec3& color) {
			if ((int)recent_items.size() >= MAX_RECENT) {
				destroy(recent_items.front());
				recent_items.erase(recent_items.begin());
			}
			Handle item = Engine::create_layout();
			Engine::set_content_size(item, {40.f, 40.f});
			Engine::set_layout_background_enabled(item, true);
			Engine::set_layout_background_color(item, color);
			Engine::set_layout_border_radius(item, 8.f);
			Engine::add_child_to_container(recent_scroll, item);
			track(item);
			set_touch_action(item, [color](Handle, Engine::TOUCH_STATE touch_state, glm::vec2) {
				if (touch_state != Engine::TOUCH_STATE::TOUCH_ENDED)
					return;
				current = color;
				refresh_preview();
				refresh_sliders();
			});
			// swallow false so a drag on an item still scrolls the list underneath.
			Engine::set_swallow_touches(item, false);
			recent_items.push_back(item);
			layout_recent();
		}

		void select_swatch(const Handle& swatch, const glm::vec3& color) {
			if (!selected_swatch.is_invalid()) {
				Engine::set_layout_border_width(selected_swatch, 0.f);
			}
			selected_swatch = swatch;
			Engine::set_layout_border_width(swatch, 3.f);
			Engine::set_layout_border_color(swatch, WHITE);
			current = color;
			refresh_preview();
			refresh_sliders();
			push_recent(color);
		}

		glm::vec3 palette_color(int column, int row) {
			if (column == COLUMNS - 1) {
				float grey = 1.f - (float)row / (float)(ROWS - 1);
				return {grey, grey, grey};
			}
			const float saturation[ROWS] = {1.f, 0.7f, 0.4f, 1.f, 1.f, 1.f};
			const float value[ROWS] = {1.f, 1.f, 1.f, 0.75f, 0.5f, 0.3f};
			return hsv_to_rgb((float)column / (float)(COLUMNS - 1), saturation[row], value[row]);
		}
	} // namespace Picker

	void build_picker_scene() {
		using namespace Picker;
		glm::vec2 window = Engine::get_window_size();
		Handle page = make_page("color picker", {0.11f, 0.12f, 0.14f});
		selected_swatch = Handle::make_invalid_handle();
		recent_items.clear();
		current = {0.22f, 0.45f, 0.85f};

		glm::vec2 size{640.f, 460.f};
		Handle panel = make_layout(page, window * 0.5f, size, {0.19f, 0.2f, 0.23f}, 16.f, {0.5f, 0.5f});
		Engine::set_layout_border_width(panel, 1.f);
		Engine::set_layout_border_color(panel, {0.3f, 0.32f, 0.36f});
		make_label(panel, "UI test 2: colour picker", 20, WHITE, {24.f, size.y - 30.f}, {0.f, 0.5f});

		// palette grid.
		glm::vec2 grid_origin{24.f, size.y - 70.f - (CELL + GAP) * ROWS};
		for (int row = 0; row < ROWS; row++) {
			for (int column = 0; column < COLUMNS; column++) {
				glm::vec3 color = palette_color(column, row);
				glm::vec2 position = grid_origin + glm::vec2(column * (CELL + GAP), (ROWS - 1 - row) * (CELL + GAP));
				Handle swatch = make_layout(panel, position, {CELL, CELL}, color, 6.f);
				set_touch_action(swatch, [color](Handle e, Engine::TOUCH_STATE touch_state, glm::vec2) {
					if (touch_state == Engine::TOUCH_STATE::TOUCH_ENDED) {
						select_swatch(e, color);
					}
				});
			}
		}

		// preview + text.
		preview = make_layout(panel, {452.f, size.y - 190.f}, {164.f, 120.f}, current, 12.f);
		Engine::set_layout_border_width(preview, 2.f);
		Engine::set_layout_border_color(preview, {0.3f, 0.32f, 0.36f});
		preview_text = make_label(preview, "Aa", 30, WHITE, {82.f, 60.f});
		hex_label = make_label(panel, "", 18, WHITE, {534.f, size.y - 206.f});
		rgb_label = make_label(panel, "", 14, {0.7f, 0.72f, 0.76f}, {534.f, size.y - 228.f});

		// sliders R / G / B.
		const char* names[3] = {"R", "G", "B"};
		for (int i = 0; i < 3; i++) {
			sliders[i] = make_slider(
				panel, names[i], {452.f, 190.f - 44.f * i}, 164.f, current[i],
				[i](float value) {
					current[i] = value;
					refresh_preview();
				},
				[](float) { push_recent(current); }
			);
		}

		// recent colours: horizontal scroll, clipped.
		make_label(panel, "Recent (scroll or drag)", 14, {0.7f, 0.72f, 0.76f}, {24.f, 96.f}, {0.f, 0.5f});
		recent_scroll = Engine::create_scroll_layout();
		Engine::set_content_size(recent_scroll, {592.f, 56.f});
		Engine::set_position(recent_scroll, {24.f, 24.f});
		Engine::set_layout_background_enabled(recent_scroll, true);
		Engine::set_layout_background_color(recent_scroll, {0.14f, 0.15f, 0.17f});
		Engine::set_layout_border_radius(recent_scroll, 10.f);
		Engine::set_scroll_direction(recent_scroll, Engine::SCROLL_DIRECTION::SCROLL_HORIZONTAL);
		Engine::set_inner_container_size(recent_scroll, {592.f, 56.f});
		Engine::set_layout_scroll_touch_enabled(recent_scroll, true);
		Engine::add_child(panel, recent_scroll);
		track(recent_scroll);

		refresh_preview();
		Log::info("[test] picker scene built, nodes:", get_scene().nodes.size());
	}

	// ============================== 3. COMPONENT LAB ==============================

	namespace Lab {
		Handle squares[3];
		uint32_t next_z = 1;
		int raise_index = 0;
		int opacity_step = 0;
		Handle measure_label = Handle::make_invalid_handle();
		Handle measure_box = Handle::make_invalid_handle();
		int text_step = 0;
		Handle nine_big = Handle::make_invalid_handle();

		void fit_measure_box() {
			// read size right after set_label_string: must already be the new size.
			glm::vec2 text_size = Engine::get_content_size(measure_label);
			Engine::set_content_size(measure_box, text_size + glm::vec2(24.f, 16.f));
			Engine::set_position(measure_label, (text_size + glm::vec2(24.f, 16.f)) * 0.5f);
			Log::info("[lab] measured label size:", text_size);
		}

		Handle make_box(const Handle& page, int column, int row, const std::string& title) {
			glm::vec2 window = Engine::get_window_size();
			glm::vec2 size{370.f, 300.f};
			glm::vec2 position{20.f + column * (size.x + 20.f), window.y - 80.f - row * (size.y + 20.f)};
			Handle box = make_layout(page, position, size, CARD, 10.f, {0.f, 1.f});
			make_label(box, title, 16, INK, {14.f, size.y - 20.f}, {0.f, 0.5f});
			return box;
		}
	} // namespace Lab

	void build_lab_scene() {
		using namespace Lab;
		glm::vec2 window = Engine::get_window_size();
		Handle page = make_page("component lab", {0.9f, 0.91f, 0.93f});
		next_z = 1;
		raise_index = 0;
		opacity_step = 0;
		text_step = 0;
		make_label(page, "UI test 3: component lab", 22, INK, {20.f, window.y - 40.f}, {0.f, 0.5f});

		// box 1: percent + pinning.
		Handle box_1 = make_box(page, 0, 0, "Percent size/position + pinning");
		Handle percent_parent = make_layout(box_1, {14.f, 140.f}, {160.f, 120.f}, {0.85f, 0.88f, 0.95f}, 6.f);
		Handle percent_child = make_layout(percent_parent, {0.f, 0.f}, {10.f, 10.f}, BLUE, 4.f, {0.5f, 0.5f});
		Engine::set_size_percent(percent_child, {0.5f, 0.5f});
		Engine::set_position_percent(percent_child, {0.5f, 0.5f});
		make_label(box_1, "blue = half size, centred", 12, MUTED, {14.f, 124.f}, {0.f, 0.5f});
		Handle pin_parent = make_layout(box_1, {196.f, 140.f}, {160.f, 120.f}, {0.95f, 0.88f, 0.85f}, 6.f);
		Handle pin_child = make_layout(pin_parent, {0.f, 0.f}, {10.f, 10.f}, RED, 4.f);
		Engine::set_size_pinning(pin_child, {20.f, 20.f, 20.f, 20.f});
		make_label(box_1, "red = 20 px inset", 12, MUTED, {196.f, 124.f}, {0.f, 0.5f});
		make_button(box_1, "Grow parents", {14.f, 20.f}, {160.f, 40.f}, BLUE, [percent_parent, pin_parent]() {
			for (const auto& parent : {percent_parent, pin_parent}) {
				glm::vec2 size = Engine::get_content_size(parent);
				Engine::set_content_size(parent, size.x > 200.f ? glm::vec2(160.f, 120.f) : size + 20.f);
			}
		});

		// box 2: nested rotated clipping, 3 levels.
		Handle box_2 = make_box(page, 1, 0, "Nested rotated clipping");
		Handle clip_1 = make_layout(box_2, {185.f, 140.f}, {280.f, 190.f}, {0.2f, 0.24f, 0.32f}, 20.f, {0.5f, 0.5f});
		Engine::set_rotation(clip_1, 8.f);
		Engine::set_layout_clipping_enabled(clip_1, true);
		Handle clip_2 = make_layout(clip_1, {140.f, 95.f}, {300.f, 120.f}, {0.3f, 0.5f, 0.75f}, 14.f, {0.5f, 0.5f});
		Engine::set_rotation(clip_2, -25.f);
		Engine::set_layout_clipping_enabled(clip_2, true);
		Handle clip_3 = make_layout(clip_2, {150.f, 60.f}, {120.f, 160.f}, {0.95f, 0.75f, 0.3f}, 30.f, {0.5f, 0.5f});
		Engine::set_rotation(clip_3, 40.f);
		Engine::set_layout_clipping_enabled(clip_3, true);
		Handle overflow_image = make_image(clip_3, "res/AddonIcon7.png", {60.f, 80.f});
		Engine::set_scale(overflow_image, {1.4f, 1.4f});
		make_label(clip_2, "text that overflows the clip on both sides", 18, WHITE, {150.f, 20.f});
		make_button(box_2, "Spin", {14.f, 14.f}, {90.f, 36.f}, BLUE, [clip_1, clip_2]() {
			Engine::set_rotation(clip_1, Engine::get_rotation(clip_1) + 15.f);
			Engine::set_rotation(clip_2, Engine::get_rotation(clip_2) - 10.f);
		});

		// box 3: nine scale.
		Handle box_3 = make_box(page, 2, 0, "Nine-scale (ratio + pixel capset)");
		const glm::vec2 nine_sizes[3] = {{80.f, 40.f}, {200.f, 60.f}, {320.f, 90.f}};
		const glm::vec2 nine_positions[3] = {{14.f, 225.f}, {110.f, 205.f}, {14.f, 105.f}};
		for (int i = 0; i < 3; i++) {
			Handle nine = make_image(box_3, "res/BtnGreen.png", nine_positions[i], {0.f, 0.f});
			Engine::set_nine_scale_enabled(nine, true);
			Engine::set_nine_scale_rect_ratio(nine, {0.25f, 0.3f, 0.5f, 0.4f});
			Engine::set_content_size(nine, nine_sizes[i]);
			Engine::set_image_nine_scale_size(nine, nine_sizes[i]);
			if (i == 2)
				nine_big = nine;
		}
		Handle pixel_nine = make_image(box_3, "res/GemsBarBg.png", {14.f, 58.f}, {0.f, 0.f});
		Engine::set_nine_scale_enabled(pixel_nine, true);
		Engine::set_nine_scale_rect(pixel_nine, {30.f, 20.f, 169.f, 22.f});
		Engine::set_content_size(pixel_nine, {340.f, 40.f});
		Engine::set_image_nine_scale_size(pixel_nine, {340.f, 40.f});
		make_button(box_3, "Toggle nine-scale", {14.f, 14.f}, {180.f, 36.f}, BLUE, []() {
			Engine::set_nine_scale_enabled(nine_big, !Engine::is_nine_scale_enabled(nine_big));
		});

		// box 4: z order + opacity.
		Handle box_4 = make_box(page, 0, 1, "Z-order + opacity");
		const glm::vec3 colors[3] = {RED, {0.3f, 0.7f, 0.4f}, BLUE};
		for (int i = 0; i < 3; i++) {
			squares[i] = make_layout(box_4, {60.f + i * 60.f, 110.f + i * 30.f}, {120.f, 120.f}, colors[i], 12.f);
			make_label(squares[i], std::string(1, (char)('A' + i)), 28, WHITE, {60.f, 60.f});
		}
		make_button(box_4, "Raise next", {14.f, 14.f}, {130.f, 36.f}, BLUE, []() {
			Engine::set_z_order(squares[raise_index], next_z++);
			raise_index = (raise_index + 1) % 3;
		});
		make_button(box_4, "Opacity B", {154.f, 14.f}, {130.f, 36.f}, BLUE, []() {
			const float values[3] = {1.f, 0.6f, 0.25f};
			opacity_step = (opacity_step + 1) % 3;
			Engine::set_opacity(squares[1], values[opacity_step]);
		});

		// box 5: label measure.
		Handle box_5 = make_box(page, 1, 1, "Label measure (box follows text)");
		measure_box = make_layout(box_5, {14.f, 150.f}, {10.f, 10.f}, {0.85f, 0.88f, 0.95f}, 8.f);
		measure_label = make_label(measure_box, "Short", 20, INK, {0.f, 0.f});
		Engine::set_label_stroke_size(measure_label, 1.f);
		Engine::set_label_stroke_color(measure_label, WHITE);
		fit_measure_box();
		make_button(box_5, "Change text", {14.f, 14.f}, {150.f, 36.f}, BLUE, []() {
			const char* texts[4] = {"Short", "A much longer label string", "Two\nlines", ""};
			const int sizes[4] = {20, 16, 26, 20};
			text_step = (text_step + 1) % 4;
			Engine::set_label_font_size(measure_label, sizes[text_step]);
			Engine::set_label_string(measure_label, texts[text_step]);
			fit_measure_box();
		});

		// box 6: scroll list + missing image.
		Handle box_6 = make_box(page, 2, 1, "Vertical scroll + missing image");
		Handle list = Engine::create_scroll_layout();
		Engine::set_content_size(list, {200.f, 250.f});
		Engine::set_position(list, {14.f, 14.f});
		Engine::set_layout_background_enabled(list, true);
		Engine::set_layout_background_color(list, {0.93f, 0.94f, 0.96f});
		Engine::set_layout_border_radius(list, 8.f);
		Engine::set_scroll_direction(list, Engine::SCROLL_DIRECTION::SCROLL_VERTICAL);
		Engine::set_layout_scroll_touch_enabled(list, true);
		const int rows = 30;
		Engine::set_inner_container_size(list, {200.f, rows * 40.f});
		Engine::add_child(box_6, list);
		track(list);
		for (int i = 0; i < rows; i++) {
			Handle row = Engine::create_layout();
			Engine::set_content_size(row, {184.f, 34.f});
			Engine::set_position(row, {8.f, rows * 40.f - 40.f * (i + 1) + 3.f});
			Engine::set_layout_background_enabled(row, true);
			Engine::set_layout_background_color(row, i % 2 ? CARD : glm::vec3(0.86f, 0.89f, 0.95f));
			Engine::set_layout_border_radius(row, 6.f);
			Engine::add_child_to_container(list, row);
			track(row);
			make_label(row, "Row " + std::to_string(i + 1), 14, INK, {12.f, 17.f}, {0.f, 0.5f});
			set_touch_action(row, [i](Handle, Engine::TOUCH_STATE touch_state, glm::vec2) {
				if (touch_state == Engine::TOUCH_STATE::TOUCH_ENDED)
					Log::info("[lab] row clicked:", i + 1);
			});
			Engine::set_swallow_touches(row, false);
		}
		make_image(box_6, "res/does_not_exist.png", {290.f, 150.f});
		make_label(box_6, "missing file ->\ndefault image", 12, MUTED, {290.f, 90.f});

		Log::info("[test] lab scene built, nodes:", get_scene().nodes.size());
	}

	// ============================== LABEL SCENE (key 4) ==============================

	namespace Label_Lab {
		const glm::vec2 CELL{260.f, 80.f};
		const char* const CYCLE_TEXTS[] = {"Hi", "Medium label", "Line one\nLine two"};

		struct Cell_Setup {
			std::string caption;
			std::string text;
			Engine::LABEL_HORIZONTAL_ALIGN horizontal = Engine::LABEL_HORIZONTAL_ALIGN_CENTER;
			Engine::LABEL_VERTICAL_ALIGN vertical = Engine::LABEL_VERTICAL_ALIGN_CENTER;
			float padding_horizontal = 0.f;
			float padding_vertical = 0.f;
			Engine::LABEL_RENDER_FIT_MODE fit = Engine::LABEL_RENDER_FIT_CLIP;
			int font_size = 18;
			float stroke = 0.f;
			bool custom_size = true;
		};

		/**
		 * @Note: a bordered cell (layout) with one label inside. Custom-size labels get the cell size, so the
		 * align / padding / fit settings decide where the text sits; the border shows the label's box.
		 */
		Handle make_cell(const Handle& page, int column, int row, const Cell_Setup& setup) {
			glm::vec2 window = Engine::get_window_size();
			glm::vec2 position{20.f + column * (CELL.x + 30.f), window.y - 90.f - row * (CELL.y + 36.f)};
			make_label(page, setup.caption, 12, MUTED, position + glm::vec2(0.f, 12.f), {0.f, 0.5f});
			Handle cell = make_layout(page, position, CELL, {0.93f, 0.95f, 1.f}, 6.f, {0.f, 1.f});
			Engine::set_layout_border_width(cell, 1.5f);
			Engine::set_layout_border_color(cell, {0.55f, 0.62f, 0.8f});
			Handle label = make_label(cell, setup.text, setup.font_size, INK, {0.f, 0.f}, {0.f, 0.f});
			Engine::set_label_horizontal_align(label, setup.horizontal);
			Engine::set_label_vertical_align(label, setup.vertical);
			Engine::set_label_padding_horizontal(label, setup.padding_horizontal);
			Engine::set_label_padding_vertical(label, setup.padding_vertical);
			Engine::set_label_render_fit_mode(label, setup.fit);
			if (setup.stroke > 0.f) {
				Engine::set_label_stroke_size(label, setup.stroke);
				Engine::set_label_stroke_color(label, BLUE);
			}
			if (setup.custom_size) {
				Engine::set_content_size(label, CELL);
			} else {
				Engine::set_label_render_fit_mode(label, Engine::LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_AUTO);
			}
			// click the cell to cycle short / medium / two-line text and watch it re-layout.
			auto step = std::make_shared<int>(0);
			Engine::set_touch_enabled(cell, true);
			Engine::set_click_caller(cell, [label, step](Handle, glm::vec2, int, int) {
				if (!Engine::is_valid(label))
					return;
				*step = (*step + 1) % 3;
				Engine::set_label_string(label, CYCLE_TEXTS[*step]);
				Log::info(
					"[label] text ->", CYCLE_TEXTS[*step], "| size", Engine::get_content_size(label).x,
					Engine::get_content_size(label).y
				);
			});
			return label;
		}
	} // namespace Label_Lab

	void build_label_scene() {
		using namespace Label_Lab;
		using H = Engine::LABEL_HORIZONTAL_ALIGN;
		using V = Engine::LABEL_VERTICAL_ALIGN;
		glm::vec2 window = Engine::get_window_size();
		Handle page = make_page("label lab", {0.9f, 0.91f, 0.93f});
		make_label(
			page, "UI test 4: label align / padding / fit (click a cell: change text)", 16, INK,
			{20.f, window.y - 30.f}, {0.f, 0.5f}
		);

		// row 0: horizontal align (vertical centre).
		make_cell(
			page, 0, 0, {"H LEFT, V CENTER", "Left", H::LABEL_HORIZONTAL_ALIGN_LEFT, V::LABEL_VERTICAL_ALIGN_CENTER}
		);
		make_cell(
			page, 1, 0,
			{"H CENTER, V CENTER", "Center", H::LABEL_HORIZONTAL_ALIGN_CENTER, V::LABEL_VERTICAL_ALIGN_CENTER}
		);
		make_cell(
			page, 2, 0, {"H RIGHT, V CENTER", "Right", H::LABEL_HORIZONTAL_ALIGN_RIGHT, V::LABEL_VERTICAL_ALIGN_CENTER}
		);
		make_cell(
			page, 3, 0, {"H LEFT, V TOP", "Top left", H::LABEL_HORIZONTAL_ALIGN_LEFT, V::LABEL_VERTICAL_ALIGN_TOP}
		);
		// row 1: vertical align (horizontal centre).
		make_cell(
			page, 0, 1, {"H CENTER, V TOP", "Top", H::LABEL_HORIZONTAL_ALIGN_CENTER, V::LABEL_VERTICAL_ALIGN_TOP}
		);
		make_cell(
			page, 1, 1,
			{"H CENTER, V CENTER", "Middle", H::LABEL_HORIZONTAL_ALIGN_CENTER, V::LABEL_VERTICAL_ALIGN_CENTER}
		);
		make_cell(
			page, 2, 1,
			{"H CENTER, V BOTTOM", "Bottom", H::LABEL_HORIZONTAL_ALIGN_CENTER, V::LABEL_VERTICAL_ALIGN_BOTTOM}
		);
		make_cell(
			page, 3, 1,
			{"H RIGHT, V BOTTOM", "Bottom right", H::LABEL_HORIZONTAL_ALIGN_RIGHT, V::LABEL_VERTICAL_ALIGN_BOTTOM}
		);
		// row 2: padding (20 px horizontal, 15 px vertical; ignored on a centred axis).
		make_cell(
			page, 0, 2,
			{"LEFT + pad h 20", "Pad left", H::LABEL_HORIZONTAL_ALIGN_LEFT, V::LABEL_VERTICAL_ALIGN_CENTER, 20.f, 0.f}
		);
		make_cell(
			page, 1, 2,
			{"RIGHT + pad h 20", "Pad right", H::LABEL_HORIZONTAL_ALIGN_RIGHT, V::LABEL_VERTICAL_ALIGN_CENTER, 20.f,
			 0.f}
		);
		make_cell(
			page, 2, 2,
			{"TOP LEFT + pad 20 / 15", "Pad top", H::LABEL_HORIZONTAL_ALIGN_LEFT, V::LABEL_VERTICAL_ALIGN_TOP, 20.f,
			 15.f}
		);
		make_cell(
			page, 3, 2,
			{"BOTTOM RIGHT + pad 20 / 15", "Pad bottom", H::LABEL_HORIZONTAL_ALIGN_RIGHT,
			 V::LABEL_VERTICAL_ALIGN_BOTTOM, 20.f, 15.f}
		);
		// row 3: multi-line, auto size (reference), stroke, big font.
		make_cell(
			page, 0, 3,
			{"multi-line (\\n), CENTER", "First line\nSecond line", H::LABEL_HORIZONTAL_ALIGN_CENTER,
			 V::LABEL_VERTICAL_ALIGN_CENTER}
		);
		Cell_Setup auto_size{"auto size (custom size off)", "Auto size"};
		auto_size.custom_size = false;
		make_cell(page, 1, 3, auto_size);
		Cell_Setup stroke{"stroke 2, font 24, CENTER", "Stroke"};
		stroke.font_size = 24;
		stroke.stroke = 2.f;
		make_cell(page, 2, 3, stroke);
		Cell_Setup big{"font 40, RIGHT / TOP", "Big", H::LABEL_HORIZONTAL_ALIGN_RIGHT, V::LABEL_VERTICAL_ALIGN_TOP};
		big.font_size = 40;
		make_cell(page, 3, 3, big);

		// overflow cases are added on demand: today the wrap path can hang (see ui_test_findings.txt).
		make_button(page, "Overflow: no spaces", {740.f, window.y - 50.f}, {190.f, 36.f}, BLUE, [page]() {
			glm::vec2 w = Engine::get_window_size();
			for (int i = 0; i < 2; i++) {
				Cell_Setup setup{
					i == 0 ? "no spaces, CLIP" : "no spaces, ELLIPSIS", "Supercalifragilisticexpialidocious_and_more"
				};
				setup.fit = i == 0 ? Engine::LABEL_RENDER_FIT_CLIP : Engine::LABEL_RENDER_FIT_ELLIPSIS;
				setup.horizontal = H::LABEL_HORIZONTAL_ALIGN_LEFT;
				make_cell(page, i, 4, setup);
			}
			(void)w;
			Log::info("[label] overflow cases without spaces added (row 5)");
		});
		make_button(page, "Overflow: wrap", {945.f, window.y - 50.f}, {190.f, 36.f}, RED, [page]() {
			for (int i = 0; i < 2; i++) {
				Cell_Setup setup{
					i == 0 ? "wrap, CLIP" : "wrap, ELLIPSIS",
					"This sentence is far too long to fit in one line of this small box at all"
				};
				setup.fit = i == 0 ? Engine::LABEL_RENDER_FIT_CLIP : Engine::LABEL_RENDER_FIT_ELLIPSIS;
				setup.horizontal = H::LABEL_HORIZONTAL_ALIGN_LEFT;
				setup.vertical = V::LABEL_VERTICAL_ALIGN_TOP;
				make_cell(page, 2 + i, 4, setup);
			}
			Log::info("[label] wrap cases added (row 5)");
		});
		Log::info("[test] label scene built, nodes:", get_scene().nodes.size());
	}

} // namespace Test
