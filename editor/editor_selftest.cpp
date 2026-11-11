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
	uint32_t spawn_touch(const glm::vec2& touch_world_position);
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

		void press(glm::vec2 world) {
			Engine::window_unique_touch_id = Engine::spawn_touch(world);
		}

		void move_to(glm::vec2 world) {
			glm::vec2 w = to_window(world);
			Engine::on_cursor_touch_position(window, w.x, w.y);
		}

		void release(glm::vec2 world) {
			Engine::release_touch(Engine::window_unique_touch_id, world);
		}

		// press at `from`, move in small steps (one per frame: the engine cancels big jumps), release at `to`.
		void drag(const std::function<glm::vec2()>& from, const std::function<glm::vec2()>& to, int moves = 12) {
			auto start = std::make_shared<glm::vec2>();
			auto end = std::make_shared<glm::vec2>();
			then(0, [=]() {
				*start = from();
				*end = to();
				press(*start);
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

		glm::vec2 vec(bool ok, const float v[2]) {
			return ok ? glm::vec2(v[0], v[1]) : glm::vec2(-1.f);
		}

		glm::vec2 cell_center(unsigned i) {
			float v[2];
			bool ok = Editor::get_cell_center(i, v);
			return vec(ok, v);
		}

		glm::vec2 canvas_center() {
			float v[2];
			bool ok = Editor::get_canvas_center(v);
			return vec(ok, v);
		}

		glm::vec2 selected_center() {
			float v[2];
			bool ok = Editor::get_selected_center(v);
			return vec(ok, v);
		}

		glm::vec2 selected_position() {
			float v[2];
			bool ok = Editor::get_selected_position(v);
			return vec(ok, v);
		}
	} // namespace

	void start(GLFWwindow* glfw_window) {
		window = glfw_window;
		Editor::set_copy_imports_to_source(false);
		// the real mouse would feed the same touch id: a jump from the fake position to the real cursor
		// is more than half the window, so the engine cancels the drag. Only fake input during the test.
		glfwSetCursorPosCallback(window, nullptr);
		glfwSetMouseButtonCallback(window, nullptr);
		then(10, [] {
			expect(Editor::get_resource_count() > 0,
				   "resource panel lists images under res/ (" + std::to_string(Editor::get_resource_count()) + ")");
		});
		screenshot("start");

		// 1. click a cell selects it (no drag, no node).
		then(0, [] { press(cell_center(0)); });
		then(2, [] { release(cell_center(0)); });
		then(3, [] { expect(Editor::get_canvas_node_count() == 0, "a click on a cell does not create a node"); });

		// 2. drag the first resource onto the canvas centre.
		drag([] { return cell_center(0); }, [] { return canvas_center(); });
		then(2, [] {
			expect(Editor::get_canvas_node_count() == 1, "drag cell -> canvas creates 1 node");
			expect(!Editor::get_selected_name().empty(), "the new node is selected: " + Editor::get_selected_name());
			glm::vec2 c = selected_center(), target = canvas_center();
			expect(glm::distance(c, target) < 2.f, "node centred where it was dropped (off by " +
													   std::to_string(glm::distance(c, target)) + " px)");
		});
		screenshot("dropped");

		// 3. drag a cell and release outside the canvas: nothing created.
		drag([] { return cell_center(1); }, [] { return cell_center(0) + glm::vec2(0.f, -40.f); }, 6);
		then(2, [] { expect(Editor::get_canvas_node_count() == 1, "drop outside the canvas creates nothing"); });

		// 4. move the node by dragging it.
		then(0, [] { saved = selected_position(); });
		drag([] { return selected_center(); }, [] { return selected_center() + glm::vec2(120.f, 60.f); });
		then(2, [] {
			glm::vec2 delta = selected_position() - saved;
			expect(glm::distance(delta, glm::vec2(120.f, 60.f)) < 1.f,
				   "node moved by the drag delta (" + std::to_string(delta.x) + ", " + std::to_string(delta.y) + ")");
		});

		// 5. keys: nudge, duplicate, delete.
		then(0, [] {
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
		then(2, [] { expect(Editor::get_canvas_node_count() == saved_count + 1, "Cmd+D duplicates the node"); });
		screenshot("moved_duplicated");
		then(0, [] { Editor::on_key(GLFW_KEY_BACKSPACE, 0); });
		then(3, [] {
			expect(Editor::get_canvas_node_count() == saved_count, "Backspace deletes the selected node");
			expect(Editor::get_selected_name().empty(), "nothing selected after delete");
		});

		// 6. Finder drop (import): a copy of a res image lands in build/res/imported and in the panel.
		then(0, [] {
			saved_count = Editor::get_resource_count();
			std::string source = Utils::get_root_path() + Editor::get_resource_path(0);
			std::string copy = "/tmp/editor_selftest_import.png";
			std::filesystem::copy_file(source, copy, std::filesystem::copy_options::overwrite_existing);
			Editor::on_files_dropped({copy, "/tmp/not_an_image.txt"});
		});
		then(3, [] {
			expect(Editor::get_resource_count() == saved_count + 1, "dropping 1 image (+1 non-image) imports 1");
			bool found = false;
			for (unsigned i = 0; i < Editor::get_resource_count(); i++)
				found |= Editor::get_resource_path(i).find("res/imported/editor_selftest_import") == 0;
			expect(found, "the import shows in the panel as res/imported/...");
		});
		screenshot("imported");

		then(0, [] {
			// leave no test file behind in build/res/imported.
			std::error_code error;
			for (auto& entry : std::filesystem::directory_iterator(Utils::get_root_path() + "res/imported", error)) {
				if (entry.path().filename().string().rfind("editor_selftest_import", 0) == 0)
					std::filesystem::remove(entry.path(), error);
			}
			Log::info("[editor-test] summary:", passed, "passed,", failed, "failed");
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
