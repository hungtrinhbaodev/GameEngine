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
            VkMemoryPropertyFlags property_flags
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
            Vulkan_Command_Callback callback = nullptr
        );

        static void copy_buffer(
            Vulkan_Commands_Mode commands_mode,
            Vulkan_Buffer& src, 
            Vulkan_Buffer& dst,
            Vulkan_Command_Callback callback = nullptr
        );

    };
}