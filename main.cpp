#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#endif

#include <core.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_texture_array.h>
#include <log.h>
#include <utils.h>
#include <vulkan/vk_draw_info.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VK_A 0x41

struct Instance_Data {
	float b;
	int c;
	char k;
	friend std::ostream& operator<<(std::ostream& os, const Instance_Data& data) {
		os << data.b << "|" << data.c << "|" << data.k;
		return os;
	}
};

int main() {
	if (!glfwInit()) {
		throw std::runtime_error("fail to init glfw!");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(1200, 720, "game", nullptr, nullptr);

	Vulkan::Init::init_vulkan_core(window, Core::global_thread_pool, Core::global_scheduler);

	uint32_t texture_id = Vulkan::texture_system.load_texture(Utils::get_root_path() + "res\\texture\\texture4.png");

	while (!glfwWindowShouldClose(window)) {

		glfwPollEvents();

		// Set up all component when start frame (reset frame of ring buffer, ...).
		// Vulkan::Process::start_frame();

		// TODO: logic of all component will be place here in future.

		// Draw all information of this current frame.
		// Vulkan::Process::draw_frame();

		// Reset all cache or work need to using in this frame.
		Vulkan::Process::end_frame();
	}

	Vulkan::Destroy::destroy_vulkan();

	glfwDestroyWindow(window);

	glfwTerminate();

#ifdef _DEBUG
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
#endif

	return 0;
}
