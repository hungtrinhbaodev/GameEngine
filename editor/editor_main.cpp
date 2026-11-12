#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <thread>

#include <GLFW/glfw3.h>
#include <editor.h>
#include <editor_selftest.h>
#include <engine/engine.h>
#include <utils.h>

namespace {
	void on_key(GLFWwindow*, int key, int, int action, int mods) {
		if (action == GLFW_PRESS || action == GLFW_REPEAT)
			Editor::on_key(key, mods);
	}

	void on_drop(GLFWwindow*, int count, const char** paths) {
		std::vector<std::string> files(paths, paths + count);
		Editor::on_files_dropped(files);
	}
} // namespace

int main(int argc, char** argv) {
	bool perf = argc > 1 && std::string(argv[1]) == "--perf";
	bool self_test = perf || (argc > 1 && std::string(argv[1]) == "--self-test");
	// TODO(engine finding 41): the label fit code loads fonts relative to the working directory. Run from the
	// executable's folder so opening build-editor/editor from Finder (cwd = home) still finds res/.
	std::error_code cwd_error;
	std::filesystem::current_path(Utils::get_root_path(), cwd_error);
	if (!glfwInit()) {
		throw std::runtime_error("fail to init glfw!");
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(1440, 900, "Scene Editor", nullptr, nullptr);

	Engine::init(window);
	// the engine owns mouse / cursor / scroll callbacks; keys and file drops are free for the editor.
	glfwSetKeyCallback(window, on_key);
	glfwSetDropCallback(window, on_drop);
	Editor::init(window);
	Engine::process_frame(0.f);
	if (perf)
		Editor_Self_Test::start_perf(window);
	else if (self_test)
		Editor_Self_Test::start(window);

	double start = Utils::now();
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		double current = Utils::now();
		float dt = std::min((float)(current - start), 0.1f);
		start = current;
		if (self_test && !Editor_Self_Test::tick())
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		Editor::tick();
		Engine::process_frame(dt);
	}
	int exit_code = self_test && Editor_Self_Test::failures() > 0 ? 1 : 0;

	Editor::destroy();
	Engine::process_frame(0.f); // clean_up is queued until the next frame
	Engine::destroy();
	glfwDestroyWindow(window);
	glfwTerminate();
#ifdef _DEBUG
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
#endif
	return exit_code;
}
