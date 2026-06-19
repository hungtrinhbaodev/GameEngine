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

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE

#include "tiny_gltf.h"

#define VK_A 0x41

#if !defined(_WIN32)
const std::string DEFAULT_PATH = "/Users/lap13994/Documents/hungtrinhbaodev/GameEngine/res/texture/";
#else
const std::string DEFAULT_PATH = "D:/engine_project/game_engine/game_engine/res/texture/";
#endif

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
	GLFWwindow* window = glfwCreateWindow(1200, 800, "game", nullptr, nullptr);

	Vulkan::Init::init_vulkan_core(window, Core::global_thread_pool, Core::global_scheduler);

	Vulkan::Texture_Array texture_array{};
	texture_array.init(10, 1024, 1024);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}

	texture_array.destroy();

	Vulkan::Destroy::destroy_vulkan();

	glfwDestroyWindow(window);

	glfwTerminate();

#ifdef _DEBUG
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
#endif

	return 0;
}
