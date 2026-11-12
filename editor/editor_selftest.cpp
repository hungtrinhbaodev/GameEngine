#include <GLFW/glfw3.h>
#include <cstdlib>
#include <editor.h>
#include <editor_selftest.h>
#include <engine/engine.h>
#include <engine/touch_handler.h>
#include <filesystem>
#include <functional>
#include <log.h>
#include <utils.h>

/**
 * @Note: engine globals used to inject mouse input without moving the real cursor (same as tests/test_drive.cpp).
 */
namespace Engine {
	extern uint32_t window_unique_touch_id;
	uint32_t spawn_touch(const glm::vec2& touch_world_position, int button, int mods);
	void release_touch(uint32_t touch_id, const glm::vec2& touch_world_position);
} // namespace Engine

namespace Editor_Self_Test {

	namespace {
		struct Step {
			int wait_frames = 0;
			std::function<void()> action = nullptr;
		};
		std::vector<Step> steps;
		size_t step_index = 0;
		int wait_left = 0;
		GLFWwindow* window = nullptr;
		int passed = 0, failed = 0, shot = 0;
		glm::vec2 saved{0.f};
		unsigned saved_count = 0;
		float saved_value = 0.f;
		std::string saved_text = "";
		const std::string SCENE_FILE = "/tmp/editor_selftest_scene.json";

		void then(int wait_frames, const std::function<void()>& action) {
			steps.push_back({wait_frames, action});
		}

		void expect(bool ok, const std::string& what) {
			ok ? passed++ : failed++;
			Log::info(ok ? "[editor-test] PASS" : "[editor-test] FAIL", what);
		}

		glm::vec2 to_window(const glm::vec2& world) {
			return {world.x, Engine::get_window_size().y - world.y};
		}

		void press(glm::vec2 world, int mods = 0) {
			Engine::window_unique_touch_id = Engine::spawn_touch(world, GLFW_MOUSE_BUTTON_LEFT, mods);
		}

		void move_to(glm::vec2 world) {
			glm::vec2 w = to_window(world);
			Engine::on_cursor_touch_position(window, w.x, w.y);
		}

		void release(glm::vec2 world) {
			Engine::release_touch(Engine::window_unique_touch_id, world);
		}

		void click(const std::function<glm::vec2()>& at) {
			auto point = std::make_shared<glm::vec2>();
			then(0, [=] {
				*point = at();
				press(*point);
			});
			then(2, [=] { release(*point); });
			then(3, nullptr);
		}

		// press at `from`, move in small steps (one per frame), release at `to`.
		void drag(const std::function<glm::vec2()>& from, const std::function<glm::vec2()>& to, int moves = 12) {
			auto start = std::make_shared<glm::vec2>();
			auto end = std::make_shared<glm::vec2>();
			then(0, [=]() {
				*start = from();
				*end = to();
				press(*start);
			});
			// a real mouse starts with a few pixels: a first 7 px step lets a draggable claim the touch before a
			// scroll layout underneath sees more than its 7.5 px threshold.
			then(1, [=]() {
				glm::vec2 direction = *end - *start;
				if (glm::length(direction) > 7.f)
					move_to(*start + glm::normalize(direction) * 7.f);
			});
			for (int i = 1; i <= moves; i++) {
				then(1, [=]() { move_to(glm::mix(*start, *end, (float)i / moves)); });
			}
			then(2, [=]() { release(*end); });
			then(3, nullptr);
		}

		void screenshot(const std::string& name) {
			then(2, [name]() {
				glfwShowWindow(window);
				glfwFocusWindow(window);
			});
			then(4, [name]() {
				int x = 0, y = 0, w = 0, h = 0;
				glfwGetWindowPos(window, &x, &y);
				glfwGetWindowSize(window, &w, &h);
				char file[96];
				std::snprintf(file, sizeof(file), "/tmp/editor_%02d_%s.png", ++shot, name.c_str());
				std::string command = "screencapture -x -R" + std::to_string(x) + "," + std::to_string(y) + "," +
									  std::to_string(w) + "," + std::to_string(h) + " '" + file + "'";
				std::system(command.c_str());
				Log::info("[editor-test] screenshot", file);
			});
		}

		using Getter = bool (*)(float[2]);
		glm::vec2 point(Getter getter) {
			float v[2];
			return getter(v) ? glm::vec2(v[0], v[1]) : glm::vec2(-1.f);
		}
		glm::vec2 cell_center(unsigned i) {
			float v[2];
			return Editor::get_cell_center(i, v) ? glm::vec2(v[0], v[1]) : glm::vec2(-1.f);
		}
		glm::vec2 node_center(const std::string& name) {
			float v[2];
			return Editor::get_node_center(name, v) ? glm::vec2(v[0], v[1]) : glm::vec2(-1.f);
		}
		glm::vec2 row_center(const std::string& name) {
			float v[2];
			return Editor::get_tree_row_center(name, v) ? glm::vec2(v[0], v[1]) : glm::vec2(-1.f);
		}
		glm::vec2 field_center(const std::string& property) {
			float v[2];
			return Editor::get_field_center(property, v) ? glm::vec2(v[0], v[1]) : glm::vec2(-1.f);
		}
		glm::vec2 canvas_center() {
			return point(Editor::get_canvas_center);
		}
		glm::vec2 selected_center() {
			return point(Editor::get_selected_center);
		}
		glm::vec2 selected_position() {
			return point(Editor::get_selected_position);
		}
	} // namespace

	void start(GLFWwindow* glfw_window) {
		window = glfw_window;
		Editor::set_copy_imports_to_source(false);
		// the real mouse would feed the same touch id: only fake input during the test.
		glfwSetCursorPosCallback(window, nullptr);
		glfwSetMouseButtonCallback(window, nullptr);
		then(10, [] {
			expect(Editor::get_resource_count() > 0,
				   "FileSystem lists images under res/ (" + std::to_string(Editor::get_resource_count()) + ")");
			expect(Editor::get_tree_row_count() == 0, "empty scene: no tree rows");
		});
		screenshot("start");

		// 1. FileSystem: a click selects, a sideways drag onto the canvas adds an image at the drop point.
		click([] { return cell_center(0); });
		then(0, [] { expect(Editor::get_canvas_node_count() == 0, "a click on a cell does not create a node"); });
		drag([] { return cell_center(0); }, [] { return canvas_center() + glm::vec2(-200.f, 120.f); });
		then(2, [] {
			expect(Editor::get_canvas_node_count() == 1, "sideways drag cell -> canvas creates 1 node");
			expect(Editor::get_tree_row_count() == 1, "the tree shows it");
			glm::vec2 c = selected_center(), target = canvas_center() + glm::vec2(-200.f, 120.f);
			expect(glm::distance(c, target) < 2.f, "centred on the drop point (off by " + std::to_string(glm::distance(c, target)) + ")");
			saved_text = Editor::get_selected_name();
		});

		// 2. + Layout, then drop an image inside the layout: it becomes the layout's child.
		then(0, [] { Editor::add_layout(); });
		then(2, [] {
			expect(Editor::get_selected_name().rfind("Layout_", 0) == 0, "+ Layout adds and selects " + Editor::get_selected_name());
		});
		drag([] { return cell_center(1); }, [] { return selected_center(); });
		then(2, [] {
			std::string image = Editor::get_selected_name();
			std::string parent = Editor::get_parent_name(image);
			expect(parent.rfind("Layout_", 0) == 0, "image dropped on a layout is its child (parent: '" + parent + "')");
		});
		then(0, [] { Editor::add_label(); });
		screenshot("layout_label");

		// 3. Scene tree: drag the first image's row onto the layout's row = reparent, same place on screen.
		then(0, [] { saved = node_center(saved_text); });
		drag([] { return row_center(saved_text); },
			 [] {
				 float v[2];
				 for (int i = 1; i < 100; i++)
					 if (Editor::get_tree_row_center("Layout_" + std::to_string(i), v))
						 return glm::vec2(v[0], v[1]);
				 return glm::vec2(-1.f);
			 });
		then(2, [] {
			std::string parent = Editor::get_parent_name(saved_text);
			expect(parent.rfind("Layout_", 0) == 0, "tree drag onto a layout row reparents (parent: '" + parent + "')");
			glm::vec2 now = node_center(saved_text);
			expect(glm::distance(now, saved) < 1.f,
				   "reparent keeps the node in place on screen (moved " + std::to_string(glm::distance(now, saved)) + " px)");
		});
		// back to the canvas root by dropping on the empty part of the tree.
		drag([] { return row_center(saved_text); }, [] { return point(Editor::get_tree_empty_point); });
		then(2, [] { expect(Editor::get_parent_name(saved_text).empty(), "drop on empty tree space = canvas root"); });

		// 4. Inspector: drag the X field right by 80 px = +80, one undo step.
		click([] { return node_center(saved_text); });
		then(0, [] { saved_value = Editor::get_property(saved_text, "X"); });
		drag([] { return field_center("X"); }, [] { return field_center("X") + glm::vec2(80.f, 0.f); }, 8);
		then(2, [] {
			float delta = Editor::get_property(saved_text, "X") - saved_value;
			expect(std::abs(delta - 80.f) < 0.5f, "dragging the X field 80 px moves the node by 80 (" + std::to_string(delta) + ")");
		});

		// 5. Undo / redo.
		then(0, [] { Editor::undo(); });
		then(2, [] {
			float x = Editor::get_property(saved_text, "X");
			expect(std::abs(x - saved_value) < 0.5f, "undo restores X (" + std::to_string(x) + ")");
			Editor::redo();
		});
		then(2, [] {
			float x = Editor::get_property(saved_text, "X");
			expect(std::abs(x - saved_value - 80.f) < 0.5f, "redo applies it again (" + std::to_string(x) + ")");
		});

		// 6. Keys: nudge, duplicate, delete.
		then(0, [] {
			Editor::select_by_name(saved_text);
			saved = selected_position();
			Editor::on_key(GLFW_KEY_RIGHT, GLFW_MOD_SHIFT);
			Editor::on_key(GLFW_KEY_UP, 0);
		});
		then(2, [] {
			glm::vec2 delta = selected_position() - saved;
			expect(glm::distance(delta, glm::vec2(10.f, 1.f)) < 0.01f, "Shift+Right then Up nudges by (10, 1)");
			saved_count = Editor::get_canvas_node_count();
			Editor::on_key(GLFW_KEY_D, GLFW_MOD_SUPER);
		});
		then(2, [] { expect(Editor::get_canvas_node_count() == saved_count + 1, "Cmd+D duplicates"); });
		then(0, [] { Editor::on_key(GLFW_KEY_BACKSPACE, 0); });
		then(3, [] {
			expect(Editor::get_canvas_node_count() == saved_count, "Backspace deletes the selected node");
			expect(Editor::get_tree_row_count() == saved_count, "tree rows follow (" + std::to_string(Editor::get_tree_row_count()) + ")");
		});
		screenshot("edited");

		// 7. Save / open round trip, and a broken file leaves the scene alone.
		then(0, [] {
			saved_text = Editor::get_scene_json();
			saved_count = Editor::get_canvas_node_count();
			expect(Editor::save_to(SCENE_FILE), "save to " + SCENE_FILE);
			Editor::select_by_name(Editor::get_selected_name());
			Editor::on_key(GLFW_KEY_BACKSPACE, 0); // change the scene
		});
		then(3, [] {
			expect(Editor::open_from(SCENE_FILE), "open " + SCENE_FILE);
		});
		then(3, [] {
			expect(Editor::get_canvas_node_count() == saved_count, "open restores the node count (" +
																	   std::to_string(Editor::get_canvas_node_count()) + ")");
			expect(Editor::get_scene_json() == saved_text, "open restores exactly what was saved (JSON equal)");
			std::FILE* bad = std::fopen("/tmp/editor_selftest_bad.json", "w");
			std::fputs("{ not json", bad);
			std::fclose(bad);
			expect(!Editor::open_from("/tmp/editor_selftest_bad.json"), "a broken file is refused");
		});
		then(3, [] {
			expect(Editor::get_scene_json() == saved_text, "a refused file leaves the scene untouched");
		});

		// 8. Finder drop (import): a copy of a res image lands in build/res/imported and in FileSystem.
		then(0, [] {
			saved_count = Editor::get_resource_count();
			std::string source = Utils::get_root_path() + Editor::get_resource_path(0);
			std::string copy = "/tmp/editor_selftest_import.png";
			std::filesystem::copy_file(source, copy, std::filesystem::copy_options::overwrite_existing);
			Editor::on_files_dropped({copy, "/tmp/not_an_image.txt"});
		});
		then(3, [] {
			expect(Editor::get_resource_count() == saved_count + 1, "dropping 1 image (+1 non-image) imports 1");
		});
		screenshot("final");

		then(0, [] {
			std::error_code error;
			for (auto& entry : std::filesystem::directory_iterator(Utils::get_root_path() + "res/imported", error)) {
				if (entry.path().filename().string().rfind("editor_selftest_import", 0) == 0)
					std::filesystem::remove(entry.path(), error);
			}
			std::filesystem::remove(SCENE_FILE, error);
			std::filesystem::remove("/tmp/editor_selftest_bad.json", error);
			Log::info("[editor-test] summary:", passed, "passed,", failed, "failed");
		});
	}

	void start_perf(GLFWwindow* glfw_window) {
		window = glfw_window;
		glfwSetCursorPosCallback(window, nullptr);
		glfwSetMouseButtonCallback(window, nullptr);
		auto layers = std::make_shared<std::vector<Handle>>();
		auto set_load = [layers](int count) {
			for (const auto& layer : *layers)
				Engine::clean_up(layer);
			layers->clear();
			glm::vec2 size = Engine::get_window_size();
			for (int i = 0; i < count; i++) {
				Handle layer = Engine::create_layout();
				Engine::set_content_size(layer, size);
				Engine::set_layout_background_enabled(layer, true);
				Engine::set_layout_background_color(layer, {0.5f, 0.5f, 0.5f});
				Engine::set_opacity(layer, 0.01f);
				Engine::set_z_order(layer, 1u << 26);
				Engine::add_child(Engine::get_root(), layer);
				layers->push_back(layer);
			}
		};
		auto measure = [](const std::string& name, const std::function<void()>& setup) {
			then(0, [name, setup] {
				Log::info("[editor-perf] case", name);
				setup();
			});
			then(120, nullptr); // the overlay averages the GPU time over 20 frames
			screenshot("perf_" + name);
		};
		measure("editor_only", [=] {
			set_load(0);
			Editor::set_ui_visible(true);
		});
		measure("empty", [=] {
			set_load(0);
			Editor::set_ui_visible(false);
		});
		measure("load80_editor", [=] {
			set_load(80);
			Editor::set_ui_visible(true);
		});
		measure("load80", [=] {
			set_load(80);
			Editor::set_ui_visible(false);
		});
		then(0, [=] {
			set_load(0);
			Log::info("[editor-perf] done");
		});
	}

	bool tick() {
		if (step_index >= steps.size())
			return false;
		if (wait_left > 0) {
			wait_left--;
			return true;
		}
		Step step = steps[step_index++];
		if (step.action)
			step.action();
		if (step_index < steps.size())
			wait_left = steps[step_index].wait_frames;
		return step_index < steps.size() || wait_left > 0;
	}

	int failures() {
		return failed;
	}

} // namespace Editor_Self_Test
