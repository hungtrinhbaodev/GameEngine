#pragma once
#include <vulkan/vulkan.h>

#include <functional>
#include <mutex>

#include <graphic/vulkan_implementation/vulkan_queue_family_indices.h>
#include <graphic/vulkan_implementation/vulkan_fences.h>

namespace Graphic {

    using Submit_Callback = std::function<void()>;

    class Vulkan_Queues {

        private:

        Vulkan_Fences* _vk_fences;

        VkDevice _vk_device;

        VkQueue _vk_present_queue = VK_NULL_HANDLE;

        VkQueue _vk_graphics_queue = VK_NULL_HANDLE;

        std::mutex _sumit_lock;

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
            Submit_Callback callback = nullptr
        );

        VkQueue get_graphics_queue();

        VkQueue get_present_queue();
    };
}