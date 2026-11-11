#include <chrono>
#include <stdexcept>
#include <thread>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <engine/engine.h>
#include <log.h>
#include <test_ui.h>
#include <utils.h>

namespace {
	void on_key(GLFWwindow* window, int key, int scancode, int action, int mods) {
		if (action != GLFW_PRESS)
			return;
		Test::handle_key(key, mods);
	}
} // namespace

/**
 * @Note: no argument = interactive. "stress", "fuzz [seed] [scene 1-3] [ops per frame]"
 * or "repro | repro-clip | repro-image | repro-scroll" runs that job and quits.
 */
int main(int argc, char** argv) {
	std::string auto_job = argc > 1 ? argv[1] : "";
	if (!glfwInit()) {
		throw std::runtime_error("fail to init glfw!");
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(1200, 720, "UI test", nullptr, nullptr);

	Engine::init(window);
	glfwSetKeyCallback(window, on_key);
	Test::start();

	bool auto_quit = false;
	if (auto_job == "repro") {
		Test::handle_key(GLFW_KEY_C, 0);
		auto_quit = true;
	} else if (auto_job == "repro-clip") {
		Test::handle_key(GLFW_KEY_X, 0);
		auto_quit = true;
	} else if (auto_job == "repro-image") {
		Test::handle_key(GLFW_KEY_E, 0);
		auto_quit = true;
	} else if (auto_job == "repro-scroll") {
		Test::handle_key(GLFW_KEY_L, 0);
		auto_quit = true;
	} else if (auto_job == "repro-touch") {
		Test::handle_key(GLFW_KEY_U, 0);
		auto_quit = true;
	} else if (auto_job == "repro-api") {
		Test::clear_scene();
		Engine::process_frame(0.f);
		Test::handle_key(GLFW_KEY_K, 0);
		auto_quit = true;
	} else if (auto_job == "repro-reentry") {
		Test::clear_scene();
		Engine::process_frame(0.f);
		Test::handle_key(GLFW_KEY_J, 0);
		auto_quit = true;
	}
	bool driving = auto_job == "drive" || auto_job == "perf" || auto_job == "label-view" || auto_job == "label-wrap";
	if (auto_job == "drive") {
		Test::start_drive(window);
		auto_quit = true;
	} else if (auto_job == "label-view") {
		Test::start_label_view(window);
		auto_quit = true;
	} else if (auto_job == "label-wrap") {
		Test::start_label_wrap(window);
		auto_quit = true;
	} else if (auto_job == "perf") {
		Test::start_perf(window);
		auto_quit = true;
	}
	// render the first scene once so jobs do not start on never-rendered nodes.
	Engine::process_frame(0.f);
	if (auto_job == "stress") {
		Test::start_stress(90);
		auto_quit = true;
	} else if (auto_job == "fuzz") {
		uint32_t seed = argc > 2 ? (uint32_t)std::stoul(argv[2]) : 1u;
		int scene = argc > 3 ? std::stoi(argv[3]) : 3;
		if (scene == 1)
			Test::build_modal_scene();
		else if (scene == 2)
			Test::build_picker_scene();
		else
			Test::build_lab_scene();
		Engine::process_frame(0.f);
		Log::info("[test] tree check right after building the scene:");
		Test::verify_tree(true);
		int ops_per_frame = argc > 4 ? std::stoi(argv[4]) : 12;
		Test::start_fuzz(seed, 300 * 12 / std::max(ops_per_frame, 1), ops_per_frame);
		auto_quit = true;
	}

	double start = Utils::now();
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		double current = Utils::now();
		float dt = std::min((float)(current - start), 0.1f);
		start = current;
		Test::tick();
		if (driving) {
			driving = Test::drive_tick();
		}
		Engine::process_frame(dt);
		if (auto_quit && !driving && !Test::is_running_job()) {
			if (auto_job == "fuzz") {
				Test::clear_scene();
				Engine::process_frame(0.f); // clean_up is queued until the next frame
				Test::verify_tree(true);
				Test::print_pool_diff(Test::get_baseline(), Test::count_pools(), "after fuzz + clear");
			}
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}
	}

	Test::clear_scene();
	Engine::destroy();
	glfwDestroyWindow(window);
	glfwTerminate();

#ifdef _DEBUG
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
#endif
	return 0;
}
