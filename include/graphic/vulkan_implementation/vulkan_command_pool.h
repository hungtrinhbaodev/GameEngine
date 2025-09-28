#pragma once
#include <vulkan/vulkan.h>

#include <functional>

#include <core/concurent_pool.hpp>
#include <graphic/vulkan_implementation/vulkan_constants.h>
#include <graphic/vulkan_implementation/vulkan_queues.h>
#include <graphic/vulkan_implementation/vulkan_queue_family_indices.h>


namespace Graphic {

    using CommandCallback = std::function<void(void*)>;

    using CommandRecord = std::function<void(VkCommandBuffer)>;

    class Vulkan_Command_Pool : public Core::Concurent_Pool<VkCommandBuffer> {

        private:

        VkDevice _vk_device;

        VkCommandPool _vk_command_pool;

        std::vector<VkCommandBuffer> _vk_draw_command_buffers;

        VkCommandBuffer _create_item();

        void _destroy_item(VkCommandBuffer& command_buffer);

        public:

        void init(VkPhysicalDevice vk_physical_device, VkDevice vk_device, VkSurfaceKHR vk_surface);

        void pooling_item(const VkCommandBuffer& command_buffer);

        void record_single_commands(
            Vulkan_Commands_Mode commands_mode,
            CommandRecord record,
            void* user_data = nullptr,
            CommandCallback callback = nullptr,
            Vulkan_Queues* queues = nullptr
        );

        VkCommandBuffer get_draw_command_buffer(int frame_id);

        void destroy();
    };

}