#pragma once

#include <vulkan/vulkan.h>

namespace Vulkan {

    struct Image {

        VkDevice device;

        VkImage image;

        VkImageView view;

        VkFormat format;

        VkDeviceMemory memory;

        VkSampler sampler;

        VkImageAspectFlags aspect_flags;

        uint32_t width;

        uint32_t height;

        VkDescriptorImageInfo descriptor;

        Image();

        Image(const Image& other);

        void make_image(
            uint32_t width,
            uint32_t height,
            VkFormat format,
            VkImageTiling tiling,
            VkImageUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkImageAspectFlags aspect_flags,
            VkPhysicalDevice physical_device = VK_NULL_HANDLE,
            VkDevice device = VK_NULL_HANDLE
        );

        void transition_image_layout(
            VkImageLayout old_layout,
            VkImageLayout new_layout
        );

        void copy_image_data(uint32_t width, uint32_t height, void* pixels);

        void destroy();

    };

}