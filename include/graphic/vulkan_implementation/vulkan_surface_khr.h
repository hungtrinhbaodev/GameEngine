#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <graphic/vulkan_implementation/vulkan_utility.h>

namespace Graphic {

    class Vulkan_Surface_KHR {

        private:

        VkSurfaceKHR _vk_surface = VK_NULL_HANDLE;

        public:

        void init(VkInstance vk_instance, GLFWwindow *window);

        void destroy(VkInstance vk_instance);

        VkSurfaceKHR get();
    };
}