#include <graphic/vulkan_implementation/vulkan_surface_khr.h>

void Graphic::Vulkan_Surface_KHR::init(VkInstance vk_instance, GLFWwindow *window) {
    // init vulkan surface
    Vulkan_Utility::vk_check_action(
        glfwCreateWindowSurface(vk_instance, window, nullptr, &_vk_surface),
        "failed to create window surface!"
    );
    
    Utility::Log::get()->log_info("Create surface success!");
}

void Graphic::Vulkan_Surface_KHR::destroy(VkInstance vk_instance) {
    // clean surface KHR
    vkDestroySurfaceKHR(vk_instance, _vk_surface, nullptr);
    Utility::Log::get()->log_info("Destroy surface success!");
}

VkSurfaceKHR Graphic::Vulkan_Surface_KHR::get() {
    return _vk_surface;
}