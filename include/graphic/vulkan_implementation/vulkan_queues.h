#pragma once
#include <vulkan/vulkan.h>

#include <functional>

#include <graphic/vulkan_implementation/vulkan_utility.h>
#include <graphic/vulkan_implementation/vulkan_queue_family_indices.h>
#include <graphic/vulkan_implementation/vulkan_fences.h>

namespace Graphic {

    using Submit_Callback = std::function<void(void*)>;

    class Vulkan_Queues {

        private:

        Vulkan_Fences* _vk_fences;

        VkQueue _vk_present_queue = VK_NULL_HANDLE;

        VkQueue _vk_graphics_queue = VK_NULL_HANDLE;

        public:

        void init_queues(
            VkPhysicalDevice vk_phyiscal_device,
            VkSurfaceKHR vk_surface,
            VkDevice vk_device,
            Vulkan_Fences* vk_fences
        );

        void submit_single_commands(
            Vulkan_Queue_Submit_Mode submit_mode, 
            VkCommandBuffer command_buffer,
            void* user_data = nullptr, 
            Submit_Callback callback = nullptr
        );

        VkQueue get_graphics_queue();

        VkQueue get_present_queue();
    };
}