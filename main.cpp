#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#endif

#include <core.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_texture.h>
#include <log.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <ktx.h>
#include <ktxvulkan.h>

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

int main()
{
    if (!glfwInit()) {
        throw std::runtime_error("fail to init glfw!");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1200, 800, "game", nullptr, nullptr);

    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

	Vulkan::Init::init_vulkan_core(
        window,
        Core::global_thread_pool,
        Core::global_scheduler
    );

    Core::global_scheduler->schedule([](long long time_ms) {
        Log::log_info("Time in one loop: ", time_ms);
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }, "task_loop", 1000);

    Vulkan::Texture texture{};
    texture.load_from(DEFAULT_PATH + "texture1.png", "texture1");

    tinygltf::Model gltfModel;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
#ifdef _WIN32
        if (GetAsyncKeyState(VK_A)) {
            Core::global_scheduler->remove_task_by_name("task_loop");
        }
#endif
    }

    texture.destroy();

	Vulkan::Destroy::destroy_vulkan();

    glfwDestroyWindow(window);

    glfwTerminate();

#ifdef _DEBUG
    std::this_thread::sleep_for(std::chrono::seconds(2));
#endif

    return 0;
}
