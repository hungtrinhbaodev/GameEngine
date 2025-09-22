#pragma once
#include <vulkan/vulkan.h>

#include <graphic/vulkan_implementation/vulkan_utility.h>
#include <graphic/vulkan_implementation/vulkan_queue_family_indices.h>

namespace Graphic {

    class Vulkan_Queues {

        private:

        VkQueue _vk_present_queue = VK_NULL_HANDLE;

        VkQueue _vk_graphics_queue = VK_NULL_HANDLE;

        public:

        void init_queues(
            VkPhysicalDevice vk_phyiscal_device,
            VkSurfaceKHR vk_surface,
            VkDevice vk_device
        );

        VkQueue get_graphics_queue();

        VkQueue get_present_queue();
    };
}