#pragma once
#include <vulkan/vulkan.h>

#include <graphic/common/window.h>
#include <graphic/vulkan_implementation/vulkan_utility.h>
#include <utility/log_utils.h>

namespace Graphic {
    
    class Vulkan_Physical_Device {

        private:

        VkPhysicalDevice _vk_physical_device = VK_NULL_HANDLE;

        public:

        void init(VkInstance vk_instance, VkSurfaceKHR vk_surface);

        VkPhysicalDevice get();
    };

}