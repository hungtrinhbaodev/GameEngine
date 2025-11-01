#pragma once
#include <vulkan/vulkan.h>

#include <vector>

#include <utility/func_utils.h>
#include <graphic/vulkan_implementation/vulkan_constants.h>
#include <graphic/vulkan_implementation/vulkan_utility.h>
#include <graphic/vulkan_implementation/vulkan_queues.h>
#include <graphic/vulkan_implementation/vulkan_command_pool.h>

namespace Graphic {

    enum Vulkan_Buffer_Map_Memory_State {
        UNMAP,
        MAPPED
    };

    class Vulkan_Buffer {

        protected:

        VkDevice _vk_device;

        VkBuffer _vk_buffer;

        VkDeviceMemory _vk_device_memory;

        uint32_t _size = 0;

        VkBufferUsageFlags _usage;

        VkMemoryPropertyFlags _property_flags;

        void* _map_ptr;

        bool _is_auto_map_memory = false;

        Vulkan_Buffer_Map_Memory_State _map_state = Vulkan_Buffer_Map_Memory_State::UNMAP;

        public:

        void make(
            uint32_t size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags property_flags,
            bool is_auto_map_memory = false
        );

        void map_memory();

        void unmap_memory();

        void copy_data(void * data_src, uint32_t size);

        void map_and_copy_data(void * data_src, uint32_t size);

        void swap_with_other(Vulkan_Buffer& other);

        VkBuffer get();

        VkDeviceSize get_size();

        VkDeviceMemory& get_vk_device_memory();

        void destroy();

        static void copy_buffer(
            Vulkan_Commands_Mode commands_mode,
            Vulkan_Buffer* src,
            Vulkan_Buffer* dst,
            const std::vector<VkBufferCopy>& copy_regions,
            Vulkan_Command_Callback callback = nullptr
        );

        static void copy_buffer(
            Vulkan_Commands_Mode commands_mode,
            Vulkan_Buffer* src, 
            Vulkan_Buffer* dst,
            Vulkan_Command_Callback callback = nullptr
        );

    };
}