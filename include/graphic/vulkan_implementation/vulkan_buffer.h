#pragma once
#include <vulkan/vulkan.h>

#include <vector>

#include <graphic/vulkan_implementation/vulkan_constants.h>
#include <graphic/vulkan_implementation/vulkan_utility.h>
#include <graphic/vulkan_implementation/vulkan_queues.h>
#include <graphic/vulkan_implementation/vulkan_command_pool.h>

namespace Graphic {

    class Vulkan_Buffer {

        private:

        VkDevice _vk_device;

        VkBuffer _vk_buffer;

        VkDeviceMemory _vk_device_memory;

        VkDeviceSize _size;

        void* _map_ptr;

        public:

        void make(
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags property_flags,
            VkPhysicalDevice vk_physical_device = VK_NULL_HANDLE,
            VkDevice vk_device = VK_NULL_HANDLE
        );

        void map_memory();

        void unmap_memory();

        void copy_data(void * data_src, size_t size);

        void map_and_copy_data(void * data_src, size_t size);

        VkBuffer get();

        VkDeviceSize get_size();

        void destroy();

        static void copy_buffer(
            Vulkan_Commands_Mode commands_mode,
            Vulkan_Buffer& src,
            Vulkan_Buffer& dst,
            const std::vector<VkBufferCopy>& copy_regions,
            void* user_data = nullptr,
            Vulkan_Command_Callback callback = nullptr,
            Vulkan_Command_Pool* vk_command_pool = nullptr,
            Vulkan_Queues* vk_queues = nullptr
        );

        static void copy_buffer(
            Vulkan_Commands_Mode commands_mode,
            Vulkan_Buffer& src, 
            Vulkan_Buffer& dst,
            void* user_data = nullptr,
            Vulkan_Command_Callback callback = nullptr,
            Vulkan_Command_Pool* vk_command_pool = nullptr,
            Vulkan_Queues* vk_queues = nullptr
        );

    };
}