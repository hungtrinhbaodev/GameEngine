#pragma once
#include <vulkan/vulkan.h>

#include <set>

#include <graphic/vulkan_implementation/vulkan_queue_family_indices.h>
#include <graphic/vulkan_implementation/vulkan_utility.h>

namespace Graphic {

    class Vulkan_Device {

        private:

        VkDevice _vk_device = VK_NULL_HANDLE;

        public:

        void init(VkPhysicalDevice vk_physical_device, VkSurfaceKHR vk_surface);

        void destroy();

        VkDevice get();
    };
}