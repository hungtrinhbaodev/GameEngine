#include <cmath>
#include <cstdlib>
#include <engine/components.h>
#include <engine/scroll_handler.h>
#include <engine/touch_handler.h>
#include <id_generator.h>
#include <log.h>
#include <math_custom.h>
#include <test_ui.h>
#include <vulkan/vk_draw.h>

/**
 * @Note: engine globals (non-static) used to inject input like a real mouse would,
 * without moving the user's cursor.
 */
namespace Engine {
	extern Handle_Generator entity_generator;
	extern uint32_t window_unique_touch_id;
	uint32_t spawn_touch(const glm::vec2& touch_world_position);
	void release_touch(uint32_t touch_id, const glm::vec2& touch_world_position);
	void on_scroll_fire(GLFWwindow* window, double xoffset, double yoffset);
} // namespace Engine

namespace Test {

	namespace {
		struct Step {
			int wait_frames = 0;
			std::function<void()> action = nullptr;
		};

		GLFWwindow* drive_window = nullptr;
		std::vector<Step> steps{};
		size_t step_index = 0;
		int wait_left = 0;
		int passed = 0;
		int failed = 0;
		int shot_index = 0;
		std::vector<std::string> failures{};

		// ------------------------------ lookups ------------------------------

		Handle handle_of(uint32_t id) {
			auto& generations = Engine::entity_generator.generations;
			if (!generations.has(id))
				return Handle::make_invalid_handle();
			return Handle{id, generations.get(id)};
		}

		bool in_tree(uint32_t id) {
			Registry& registry = Engine::get_registry();
			if (!registry.has<Engine::Tree_Information>(id))
				return false;
			return std::get<0>(registry.read<Engine::Tree_Information>(id)).valid_child;
		}

		/**
		 * @Note: first label in the tree whose text starts with prefix.
		 */
		Handle find_label(const std::string& prefix) {
			Registry& registry = Engine::get_registry();
			Handle found = Handle::make_invalid_handle();
			registry.each<Engine::Label_Renderer_Information>([&](uint32_t e,
																  Engine::Label_Renderer_Information& info) {
				if (found.is_invalid() && in_tree(e) && info.text.rfind(prefix, 0) == 0)
					found = handle_of(e);
			});
			return found;
		}

		Handle parent_of(const Handle& e) {
			Registry& registry = Engine::get_registry();
			if (e.is_invalid() || !registry.has<Engine::Tree_Information>(e.id()))
				return Handle::make_invalid_handle();
			return std::get<0>(registry.read<Engine::Tree_Information>(e.id())).parent;
		}

		std::string to_text(const glm::vec2& value) {
			return std::to_string(value.x) + "x" + std::to_string(value.y);
		}

		std::vector<std::string> split_lines(const std::string& text) {
			std::vector<std::string> lines;
			size_t start = 0;
			while (start <= text.size()) {
				size_t end = text.find('\n', start);
				if (end == std::string::npos)
					end = text.size();
				lines.push_back(text.substr(start, end - start));
				start = end + 1;
			}
			return lines;
		}

		/**
		 * @Note: the lab's measure label: child of the layout that sits next to "Change text" in box 5
		 * and whose text is one of the button's texts (it can be "", so find_label cannot be used).
		 */
		Handle find_measure_label() {
			Handle box_5 = parent_of(parent_of(find_label("Change text")));
			if (box_5.is_invalid())
				return Handle::make_invalid_handle();
			const std::string texts[4] = {"Short", "A much longer label string", "Two\nlines", ""};
			Registry& registry = Engine::get_registry();
			for (const auto& child : Engine::get_children(box_5)) {
				for (const auto& grand_child : Engine::get_children(child)) {
					if (!registry.has<Engine::Label_Renderer_Information>(grand_child.id()))
						continue;
					std::string text = Engine::get_label_string(grand_child);
					for (const auto& candidate : texts) {
						if (text == candidate)
							return grand_child;
					}
				}
			}
			return Handle::make_invalid_handle();
		}

		/**
		 * @Note: tree nodes whose content size equals size, sorted by world y (top first).
		 */
		std::vector<Handle> find_by_size(const glm::vec2& size) {
			Registry& registry = Engine::get_registry();
			std::vector<Handle> result{};
			registry.each<Engine::Size>([&](uint32_t e, Engine::Size& c_size) {
				if (in_tree(e) && glm::distance(c_size.value, size) < 0.5f)
					result.push_back(handle_of(e));
			});
			return result;
		}

		Handle find_scroll_layout() {
			Registry& registry = Engine::get_registry();
			Handle found = Handle::make_invalid_handle();
			registry.each<Engine::Scroll_Information>([&](uint32_t e, Engine::Scroll_Information&) {
				if (found.is_invalid() && in_tree(e))
					found = handle_of(e);
			});
			return found;
		}

		/**
		 * @Note: world center of a node: position is the anchor point, so move by
		 * (0.5 - anchor) * size * scale, rotated like the engine does.
		 */
		glm::vec2 world_center(const Handle& e) {
			Registry& registry = Engine::get_registry();
			if (e.is_invalid() || !registry.has_all<Engine::World_Transform_2D, Engine::Size>(e.id()))
				return {-1.f, -1.f};
			const auto& [transform, c_size] = registry.read<Engine::World_Transform_2D, Engine::Size>(e.id());
			glm::vec2 offset = (glm::vec2(0.5f) - transform.anchor) * c_size.value * transform.scale;
			return transform.position + Math::rotate(transform.rotation, offset);
		}

		bool rect_visible(const Handle& layout) {
			Registry& registry = Engine::get_registry();
			if (layout.is_invalid() || !registry.has<Engine::Renderer_Handles>(layout.id()))
				return false;
			const auto& [handles] = registry.read<Engine::Renderer_Handles>(layout.id());
			if (handles.value.empty() || handles.value[0].is_invalid())
				return false;
			return Vulkan::modify_rectangle(handles.value[0]).is_visible();
		}

		std::string status_text() {
			Handle status = find_label("Status:");
			return status.is_invalid() ? std::string("<no status label>") : Engine::get_label_string(status);
		}

		Handle container_of(const Handle& scroll) {
			Registry& registry = Engine::get_registry();
			return std::get<0>(registry.read<Engine::Scroll_Information>(scroll.id())).container_handle;
		}

		size_t child_count(const Handle& e) {
			Registry& registry = Engine::get_registry();
			return std::get<0>(registry.read<Engine::Tree_Information>(e.id())).children.size();
		}

		// ------------------------------ checks ------------------------------

		void expect(bool condition, const std::string& name, const std::string& detail = "") {
			if (condition) {
				passed++;
				Log::info("[drive] PASS", name);
			} else {
				failed++;
				failures.push_back(name + (detail.empty() ? "" : " (" + detail + ")"));
				Log::info("[drive] FAIL", name, detail);
			}
		}

		void expect_status(const std::string& expected, const std::string& name) {
			std::string actual = status_text();
			expect(actual == expected, name, "status = \"" + actual + "\"");
		}

		// ------------------------------ input ------------------------------

		/**
		 * @Note: screen (window) coordinates have y down, world has y up.
		 */
		glm::vec2 to_window(const glm::vec2& world) {
			return {world.x, Engine::get_window_size().y - world.y};
		}

		void then(int wait_frames, const std::function<void()>& action) {
			steps.push_back({wait_frames, action});
		}

		void press(const glm::vec2& world) {
			Engine::window_unique_touch_id = Engine::spawn_touch(world);
		}

		void move(const glm::vec2& world) {
			glm::vec2 window = to_window(world);
			Engine::on_cursor_touch_position(drive_window, window.x, window.y);
		}

		void release(const glm::vec2& world) {
			Engine::release_touch(Engine::window_unique_touch_id, world);
		}

		/**
		 * @Note: press, hold 2 frames, release on the same point, like a short mouse click.
		 */
		void click_at(const std::function<glm::vec2()>& where, const std::string& name) {
			auto point = std::make_shared<glm::vec2>(0.f);
			then(0, [where, point, name]() {
				*point = where();
				Log::info("[drive] click", name, "at", *point);
				Registry& registry = Engine::get_registry();
				for (const auto& hit : Engine::find_entites_listen_touch_at(*point)) {
					glm::vec2 size = registry.has<Engine::Size>(hit.id())
										 ? std::get<0>(registry.read<Engine::Size>(hit.id())).value
										 : glm::vec2(-1.f);
					int type = registry.has<Engine::Description>(hit.id())
								   ? (int)std::get<0>(registry.read<Engine::Description>(hit.id())).value
								   : -1;
					Log::info("[drive]    hits entity", hit.id(), "type", type, "size", size);
				}
				press(*point);
			});
			then(2, [point]() { release(*point); });
			then(4, nullptr);
		}

		void click_label(const std::string& text) {
			click_at([text]() { return world_center(parent_of(find_label(text))); }, "\"" + text + "\"");
		}

		void drag(const std::function<glm::vec2()>& where, const glm::vec2& delta, const std::string& name) {
			auto point = std::make_shared<glm::vec2>(0.f);
			then(0, [where, point, name]() {
				*point = where();
				Log::info("[drive] drag", name, "from", *point);
				press(*point);
			});
			for (int i = 1; i <= 5; i++) {
				then(1, [point, delta, i]() { move(*point + delta * ((float)i / 5.f)); });
			}
			then(1, [point, delta]() { release(*point + delta); });
			then(4, nullptr);
		}

		void wheel(const std::function<glm::vec2()>& where, float amount) {
			then(0, [where, amount]() {
				glm::vec2 window = to_window(where());
				Engine::on_cursor_scroll_position(drive_window, window.x, window.y);
				Engine::on_scroll_fire(drive_window, 0.0, amount);
			});
			then(3, nullptr);
		}

		void screenshot(const std::string& name) {
			then(3, [name]() {
				const char* dir = std::getenv("UI_TEST_SHOTS");
				std::string folder = dir ? dir : "/tmp";
				int x = 0, y = 0, w = 0, h = 0;
				glfwGetWindowPos(drive_window, &x, &y);
				glfwGetWindowSize(drive_window, &w, &h);
				char file[64];
				std::snprintf(file, sizeof(file), "%02d_%s.png", ++shot_index, name.c_str());
				std::string command = "screencapture -x -R" + std::to_string(x) + "," + std::to_string(y) + "," +
									  std::to_string(w) + "," + std::to_string(h) + " '" + folder + "/" + file + "'";
				std::system(command.c_str());
				Log::info("[drive] screenshot", folder + "/" + file);
			});
		}

		// ------------------------------ scenario ------------------------------

		void build_modal_steps() {
			then(0, []() { build_modal_scene(); });
			then(6, nullptr);
			then(0, []() {
				// page and header are size-pinned: page = whole window, header = 64 px bar at the top.
				glm::vec2 window = Engine::get_window_size();
				Handle page = get_scene().root;
				glm::vec2 page_size = Engine::get_content_size(page);
				expect(
					glm::distance(page_size, window) < 1.f, "pinned page fills the window",
					"page " + std::to_string(page_size.x) + "x" + std::to_string(page_size.y)
				);
				Handle header = parent_of(find_label("UI test 1"));
				glm::vec2 header_size = Engine::get_content_size(header);
				expect(
					glm::distance(header_size, glm::vec2(window.x, 64.f)) < 1.f, "pinned header is a 64 px bar",
					"header " + std::to_string(header_size.x) + "x" + std::to_string(header_size.y)
				);
				float header_top = world_center(header).y + header_size.y * 0.5f;
				expect(
					std::abs(header_top - window.y) < 1.f, "pinned header sits at the top",
					"top edge at y " + std::to_string(header_top) + " of " + std::to_string(window.y)
				);
				expect(rect_visible(page), "page background visible");
			});
			screenshot("modal_page");

			click_label("Open dialog");
			then(0, []() {
				expect_status("Status: opened (visible mode)", "modal opens in visible mode");
				Handle overlay = parent_of(parent_of(find_label("Delete 3 files?")));
				expect(rect_visible(overlay), "overlay background visible on first open");
			});
			screenshot("modal_open");

			click_label("Cancel");
			then(0, []() { expect_status("Status: cancelled", "Cancel closes the dialog"); });
			screenshot("modal_closed");

			click_label("Nine-scale button");
			then(0, []() {
				expect_status("Status: nine-scale image clicked 1 time(s)", "click passes through the hidden overlay");
			});

			// where Cancel was: hidden, must not react.
			click_at([]() { return world_center(parent_of(find_label("Cancel"))); }, "hidden Cancel spot");
			then(0, []() {
				expect_status("Status: nine-scale image clicked 1 time(s)", "hidden Cancel button does not react");
			});

			click_label("Open dialog");
			then(0, []() {
				Handle title = find_label("Delete 3 files?");
				Handle dialog = parent_of(title);
				Handle overlay = parent_of(dialog);
				expect(rect_visible(overlay), "overlay background back after reopen");
				expect(rect_visible(dialog), "dialog panel background back after reopen");
				expect(rect_visible(parent_of(find_label("Confirm"))), "button background back after reopen");
			});
			screenshot("modal_reopened");

			click_label("Confirm");
			then(0, []() { expect_status("Status: confirmed 1 time(s)", "Confirm counts and closes"); });

			click_label("Build dialog");
			then(0, []() { expect_status("Status: opened (destroy mode)", "destroy mode opens a new dialog"); });
			click_label("Destroy self");
			then(0, []() {
				expect_status(
					"Status: modal destroyed itself inside its callback", "dialog destroys itself in its callback"
				);
				expect(find_label("Delete 3 files?").is_invalid(), "destroyed dialog is gone from the tree");
				expect(verify_tree(false) == 0, "tree valid after self destroy");
			});
			screenshot("modal_after_destroy");
		}

		void build_picker_steps() {
			then(0, []() { build_picker_scene(); });
			then(6, nullptr);
			screenshot("picker");

			// swatches sorted top-left first; pick a few different ones.
			for (int pick : {0, 5, 17, 40, 66}) {
				click_at(
					[pick]() {
						auto swatches = find_by_size({30.f, 30.f});
						std::sort(swatches.begin(), swatches.end(), [](const Handle& a, const Handle& b) {
							glm::vec2 ca = world_center(a), cb = world_center(b);
							return std::abs(ca.y - cb.y) > 1.f ? ca.y > cb.y : ca.x < cb.x;
						});
						return pick < (int)swatches.size() ? world_center(swatches[pick]) : glm::vec2(-1.f);
					},
					"swatch " + std::to_string(pick)
				);
			}
			then(0, []() {
				Handle hex = find_label("#");
				expect(
					!hex.is_invalid() && Engine::get_label_string(hex).size() == 7, "hex label shows a colour",
					hex.is_invalid() ? "no hex label" : Engine::get_label_string(hex)
				);
				Handle scroll = find_scroll_layout();
				expect(
					!scroll.is_invalid() && child_count(container_of(scroll)) == 5, "5 recent colours added",
					scroll.is_invalid() ? "no scroll" : std::to_string(child_count(container_of(scroll)))
				);
			});

			auto hex_before = std::make_shared<std::string>();
			then(0, [hex_before]() { *hex_before = Engine::get_label_string(find_label("#")); });
			drag(
				[]() {
					auto knobs = find_by_size({14.f, 22.f});
					std::sort(knobs.begin(), knobs.end(), [](const Handle& a, const Handle& b) {
						return world_center(a).y > world_center(b).y;
					});
					return knobs.empty() ? glm::vec2(-1.f) : world_center(knobs[0]);
				},
				{60.f, 0.f}, "R knob"
			);
			then(0, [hex_before]() {
				std::string hex_after = Engine::get_label_string(find_label("#"));
				expect(
					hex_after != *hex_before, "dragging R knob changes the colour", *hex_before + " -> " + hex_after
				);
			});
			screenshot("picker_after_picks");

			// fill the recent row past its width, then wheel it.
			for (int pick = 0; pick < 12; pick++) {
				click_at(
					[pick]() {
						auto swatches = find_by_size({30.f, 30.f});
						return pick < (int)swatches.size() ? world_center(swatches[pick]) : glm::vec2(-1.f);
					},
					"swatch fill " + std::to_string(pick)
				);
			}
			auto container_x = std::make_shared<float>(0.f);
			then(0, [container_x]() { *container_x = Engine::get_position(container_of(find_scroll_layout())).x; });
			// content starts at the clamp edge (x = 0), so scroll toward the far end.
			wheel([]() { return world_center(find_scroll_layout()); }, 3.f);
			wheel([]() { return world_center(find_scroll_layout()); }, 3.f);
			then(0, [container_x]() {
				float x = Engine::get_position(container_of(find_scroll_layout())).x;
				expect(
					x != *container_x, "mouse wheel scrolls the recent row",
					std::to_string(*container_x) + " -> " + std::to_string(x)
				);
			});
			screenshot("picker_scrolled");
		}

		void build_lab_steps() {
			then(0, []() { build_lab_scene(); });
			then(6, nullptr);
			then(0, []() {
				// box 1: percent child = half of 160x120 and centred, pinned child = 20 px inset.
				auto parents = find_by_size({160.f, 120.f});
				expect(parents.size() == 2, "lab has both percent and pinning parents", std::to_string(parents.size()));
				auto halves = find_by_size({80.f, 60.f});
				expect(halves.size() == 1, "size percent gives half size", std::to_string(halves.size()));
				if (halves.size() == 1) {
					glm::vec2 child = world_center(halves[0]);
					glm::vec2 parent = world_center(parent_of(halves[0]));
					expect(
						glm::distance(child, parent) < 1.f, "position percent centres the child",
						"child centre off by " + std::to_string(glm::distance(child, parent))
					);
				}
				expect(find_by_size({120.f, 80.f}).size() == 1, "pinning gives a 20 px inset");
			});
			screenshot("lab");

			// same table as the lab's "Change text" button: step i shows texts[(i + 1) % 4].
			static const char* measure_texts[4] = {"Short", "A much longer label string", "Two\nlines", ""};
			static const int measure_sizes[4] = {20, 16, 26, 20};
			auto short_width = std::make_shared<float>(0.f);
			then(0, [short_width]() {
				Handle label = find_measure_label();
				*short_width = label.is_invalid() ? 0.f : Engine::get_content_size(label).x;
				expect(
					*short_width > 0.f, "measure label starts with a measured width",
					"label " + std::string(label.is_invalid() ? "not found" : "width 0")
				);
			});
			for (int i = 0; i < 4; i++) {
				click_label("Change text");
				then(0, [i, short_width]() {
					int index = (i + 1) % 4;
					std::string step = ", step " + std::to_string(i + 1);
					Handle label = find_measure_label();
					if (label.is_invalid()) {
						expect(false, "measure label found" + step);
						return;
					}
					std::string text = Engine::get_label_string(label);
					glm::vec2 measured = Engine::get_content_size(label);
					glm::vec2 box = Engine::get_content_size(parent_of(label));
					expect(text == measure_texts[index], "label string changed" + step, "\"" + text + "\"");
					expect(
						Engine::get_label_font_size(label) == measure_sizes[index], "label font size changed" + step,
						std::to_string(Engine::get_label_font_size(label))
					);
					expect(
						glm::distance(measured + glm::vec2(24.f, 16.f), box) < 0.5f,
						"measure box follows label text" + step, "label " + to_text(measured) + ", box " + to_text(box)
					);

					// size N means N units per line, plus the stroke on both sides.
					int lines = 0;
					for (const auto& line : split_lines(text))
						lines += line.empty() ? 0 : 1;
					float line_height = measure_sizes[index] + 2.f * Engine::get_label_stroke_size(label);
					float expected_height = lines * line_height;
					expect(
						std::abs(measured.y - expected_height) < 0.5f,
						"label height = lines x (font size + 2 x stroke)" + step,
						to_text(measured) + ", expected height " + std::to_string(expected_height)
					);
					if (lines == 0) {
						expect(measured.x == 0.f, "empty label measures 0 wide" + step, to_text(measured));
					}
					// text changed, so the size must not be the stale one from the previous step.
					if (index == 1) {
						expect(
							measured.x > *short_width * 2.f, "longer text measures wider than \"Short\"" + step,
							to_text(measured) + " vs Short width " + std::to_string(*short_width)
						);
					}
					if (index == 0) {
						expect(
							std::abs(measured.x - *short_width) < 0.5f,
							"\"Short\" measures the same width again" + step,
							to_text(measured) + " vs " + std::to_string(*short_width)
						);
					}
				});
			}

			click_label("Raise next");
			then(0, []() {
				expect(Engine::get_z_order(parent_of(find_label("A"))) > 0, "Raise next raises square A");
			});

			// keep the handle: turning nine-scale off may change the content size.
			auto nine_big = std::make_shared<Handle>(Handle::make_invalid_handle());
			then(0, [nine_big]() {
				auto big = find_by_size({320.f, 90.f});
				*nine_big = big.empty() ? Handle::make_invalid_handle() : big[0];
			});
			click_label("Toggle nine-scale");
			then(0, [nine_big]() {
				expect(!Engine::is_nine_scale_enabled(*nine_big), "Toggle nine-scale turns it off");
			});
			click_label("Toggle nine-scale");
			then(0, [nine_big]() {
				glm::vec2 size = Engine::get_content_size(*nine_big);
				expect(Engine::is_nine_scale_enabled(*nine_big), "Toggle nine-scale turns it back on");
				expect(
					glm::distance(size, glm::vec2(320.f, 90.f)) < 0.5f, "nine-scale keeps its 320x90 size after off/on",
					std::to_string(size.x) + "x" + std::to_string(size.y)
				);
			});

			auto rotation_before = std::make_shared<float>(0.f);
			then(0, [rotation_before]() {
				auto clip = find_by_size({280.f, 190.f});
				*rotation_before = clip.empty() ? 0.f : Engine::get_rotation(clip[0]);
			});
			click_label("Spin");
			then(0, [rotation_before]() {
				auto clip = find_by_size({280.f, 190.f});
				float after = clip.empty() ? 0.f : Engine::get_rotation(clip[0]);
				expect(std::abs(after - *rotation_before - 15.f) < 0.01f, "Spin rotates the clip box by 15");
			});

			auto list_y = std::make_shared<float>(0.f);
			then(0, [list_y]() { *list_y = Engine::get_position(container_of(find_scroll_layout())).y; });
			wheel([]() { return world_center(find_scroll_layout()); }, 4.f);
			then(0, [list_y]() {
				float y = Engine::get_position(container_of(find_scroll_layout())).y;
				expect(
					y != *list_y, "mouse wheel scrolls the vertical list",
					std::to_string(*list_y) + " -> " + std::to_string(y)
				);
			});
			then(0, [list_y]() { *list_y = Engine::get_position(container_of(find_scroll_layout())).y; });
			drag([]() { return world_center(find_scroll_layout()); }, {0.f, 80.f}, "list content");
			then(0, [list_y]() {
				float y = Engine::get_position(container_of(find_scroll_layout())).y;
				expect(
					y != *list_y, "dragging scrolls the vertical list",
					std::to_string(*list_y) + " -> " + std::to_string(y)
				);
			});
			screenshot("lab_after_actions");
		}
	} // namespace

	namespace {
		std::vector<Handle> perf_nodes{};

		void perf_set_all_clipping(bool enabled) {
			Registry& registry = Engine::get_registry();
			std::vector<uint32_t> ids;
			registry.each<Engine::Clipping_Information>([&ids](uint32_t e, Engine::Clipping_Information&) {
				ids.push_back(e);
			});
			int changed = 0;
			for (uint32_t id : ids) {
				Handle e = handle_of(id);
				if (e.is_invalid() || !in_tree(id) || e.id() == Engine::get_root().id())
					continue;
				Engine::set_layout_clipping_enabled(e, enabled);
				changed++;
			}
			Log::info("[perf] clipping", enabled ? "on" : "off", "for", changed, "nodes");
		}

		/**
		 * @Note: `layers` full-window layouts with a background, each under `clip_depth` nested full-window
		 * clipping parents, so every pixel of every layer runs the clip test clip_depth times.
		 */
		void perf_build_layers(int layers, int clip_depth, float opacity = 0.15f) {
			clear_scene();
			for (const auto& e : perf_nodes) {
				if (Engine::is_valid(e))
					Engine::clean_up(e);
			}
			perf_nodes.clear();
			glm::vec2 size = Engine::get_window_size();
			for (int i = 0; i < layers; i++) {
				Handle parent = Engine::get_root();
				for (int d = 0; d < clip_depth; d++) {
					Handle clip = Engine::create_layout();
					Engine::set_content_size(clip, size);
					Engine::set_layout_clipping_enabled(clip, true);
					Engine::add_child(parent, clip);
					if (d == 0)
						perf_nodes.push_back(clip);
					parent = clip;
				}
				Handle layer = Engine::create_layout();
				Engine::set_content_size(layer, size);
				Engine::set_layout_background_enabled(layer, true);
				Engine::set_layout_background_color(layer, {0.1f + 0.05f * i, 0.2f, 0.3f});
				Engine::set_opacity(layer, opacity);
				Engine::add_child(parent, layer);
				if (clip_depth == 0)
					perf_nodes.push_back(layer);
			}
			Log::info("[perf] built", layers, "full-window layers, clip depth", clip_depth);
		}
	} // namespace

	/**
	 * @Note: GPU cost comparison read from the on-screen profiling labels (averaged over 20 frames),
	 * one screenshot per case after the numbers settle.
	 */
	void start_perf(GLFWwindow* window) {
		drive_window = window;
		steps.clear();
		step_index = 0;
		wait_left = 0;
		passed = failed = shot_index = 0;
		failures.clear();
		glfwShowWindow(window);
		glfwFocusWindow(window);
		const int settle = 90;
		auto measure = [settle](const std::string& name, const std::function<void()>& setup) {
			then(0, [setup, name]() {
				Log::info("[perf] case", name);
				setup();
			});
			then(settle, []() {
				// keep the window on top so screencapture gets the app, not another window.
				glfwShowWindow(drive_window);
				glfwFocusWindow(drive_window);
			});
			screenshot("perf_" + name);
		};
		measure("empty", []() { clear_scene(); });
		measure("modal", []() { build_modal_scene(); });
		measure("picker_clip_on", []() { build_picker_scene(); });
		measure("picker_clip_off", []() { perf_set_all_clipping(false); });
		measure("lab_clip_on", []() { build_lab_scene(); });
		measure("lab_clip_off", []() { perf_set_all_clipping(false); });
		measure("layers10_clip0", []() { perf_build_layers(10, 0); });
		measure("layers10_clip3", []() { perf_build_layers(10, 3); });
		measure("layers20_clip0", []() { perf_build_layers(20, 0); });
		measure("layers10_opaque", []() { perf_build_layers(10, 0, 1.f); });
		// push until the frame rate drops: then the GPU is at full clock and fps shows the real limit.
		measure("layers40_clip0", []() { perf_build_layers(40, 0); });
		measure("layers80_clip0", []() { perf_build_layers(80, 0); });
		measure("layers160_clip0", []() { perf_build_layers(160, 0); });
		measure("layers80_clip3", []() { perf_build_layers(80, 3); });
		then(0, []() {
			for (const auto& e : perf_nodes) {
				if (Engine::is_valid(e))
					Engine::clean_up(e);
			}
			perf_nodes.clear();
			clear_scene();
			Log::info("[drive] summary:", 0, "passed,", 0, "failed");
		});
		Log::info("[perf] start,", steps.size(), "steps");
	}

	/**
	 * @Note: build the label lab, screenshot it, then add the overflow cases without spaces and screenshot
	 * again (the wrap cases are left out: they can hang the current wrap code).
	 */
	// label-wrap: build the label lab and click the red "Overflow: wrap" button (repro for the wrap hang, #34a).
	void start_label_wrap(GLFWwindow* window) {
		drive_window = window;
		steps.clear();
		step_index = 0;
		wait_left = 0;
		passed = failed = shot_index = 0;
		failures.clear();
		glfwShowWindow(window);
		glfwFocusWindow(window);
		then(0, []() { build_label_scene(); });
		then(10, nullptr);
		click_label("Overflow: wrap");
		then(10, nullptr);
		screenshot("label_overflow_wrap");
		then(0, []() {
			clear_scene();
			Log::info("[drive] summary:", passed, "passed,", failed, "failed");
		});
	}

	void start_label_view(GLFWwindow* window) {
		drive_window = window;
		steps.clear();
		step_index = 0;
		wait_left = 0;
		passed = failed = shot_index = 0;
		failures.clear();
		glfwShowWindow(window);
		glfwFocusWindow(window);
		then(0, []() { build_label_scene(); });
		then(10, []() {
			// what the engine computed for every custom-size label (align / padding / fit result).
			Registry& registry = Engine::get_registry();
			registry.each<Engine::Label_Renderer_Information>(
				[&registry](uint32_t e, Engine::Label_Renderer_Information& info) {
					if (info.fit_mode != Engine::LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_AUTO || !in_tree(e))
						return;
					glm::vec2 size = std::get<0>(registry.read<Engine::Size>(e)).value;
					Log::info(
						"[label] node", e, "text", info.text, "| h", (int)info.horizontal_align, "v",
						(int)info.vertical_align, "fit", (int)info.fit_mode, "pad", info.horizontal_padding,
						info.vertical_padding, "| size", size, "| inner pos", info.inner_label_transform.position,
						"anchor", info.inner_label_transform.anchor, "| clip", info.clipping_enabled
					);
				}
			);
		});
		screenshot("label_scene");
		click_label("Overflow: no spaces");
		then(10, nullptr);
		screenshot("label_overflow_no_spaces");
		then(0, []() {
			clear_scene();
			Log::info("[drive] summary:", passed, "passed,", failed, "failed");
		});
	}

	void start_drive(GLFWwindow* window) {
		drive_window = window;
		steps.clear();
		step_index = 0;
		wait_left = 0;
		passed = failed = shot_index = 0;
		failures.clear();
		glfwShowWindow(window);
		glfwFocusWindow(window);
		build_modal_steps();
		build_picker_steps();
		build_lab_steps();
		then(0, []() {
			clear_scene();
			Engine::process_frame(0.f); // clean_up is queued until the next frame
			expect(verify_tree(true) == 0, "tree valid after clearing the scene");
			expect(print_pool_diff(get_baseline(), count_pools(), "after drive") == 0, "no leaks after drive");
		});
		Log::info("[drive] start,", steps.size(), "steps");
	}

	bool drive_tick() {
		if (step_index >= steps.size())
			return false;
		if (wait_left > 0) {
			wait_left--;
			return true;
		}
		const Step& step = steps[step_index++];
		// frames have run since the last action: the draw order must match the tree again.
		static bool logged_draw_order = false;
		if (step.wait_frames > 0 && !logged_draw_order && check_draw_order(false) > 0) {
			logged_draw_order = true;
			expect(false, "draw order matches the tree before step " + std::to_string(step_index - 1));
			check_draw_order(true);
		}
		if (step.action) {
			try {
				step.action();
			} catch (const std::exception& exception) {
				expect(false, "step threw", exception.what());
			}
		}
		if (step_index < steps.size())
			wait_left = steps[step_index].wait_frames;
		if (step_index >= steps.size()) {
			Log::info("[drive] summary:", passed, "passed,", failed, "failed");
			for (const auto& failure : failures) {
				Log::info("[drive]   failed:", failure);
			}
		}
		return step_index < steps.size();
	}

} // namespace Test
