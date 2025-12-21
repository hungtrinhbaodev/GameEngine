#pragma once
#include <vulkan/vulkan.h>

namespace Vulkan {

    struct Buffer {

        VkDevice device;

        VkPhysicalDevice physical_device;

        VkBuffer buffer;

        VkDeviceMemory memory;

        uint32_t size;

        VkBufferUsageFlags usage_flags;

        VkMemoryPropertyFlags property_flags;

        Buffer();

        Buffer(const Buffer& other);

        void make_buffer(
            uint32_t size,
            VkBufferUsageFlags usage_flags,
            VkMemoryPropertyFlags property_flags,
            VkPhysicalDevice physical_device = VK_NULL_HANDLE,
            VkDevice device = VK_NULL_HANDLE
        );

        void copy_data(uint32_t size, void* data);

        void destroy(VkDevice device = VK_NULL_HANDLE);

    };

}