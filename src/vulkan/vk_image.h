#pragma once

#include <vulkan/vulkan.h>

namespace Vulkan {

    struct Image {

        VkDevice device = VK_NULL_HANDLE;
        
        VkPhysicalDevice physical_device = VK_NULL_HANDLE;

        VkImage image = VK_NULL_HANDLE;

        VkImageView view = VK_NULL_HANDLE;

        VkFormat format = VK_FORMAT_UNDEFINED;

        VkDeviceMemory memory = VK_NULL_HANDLE;

        VkSampler sampler = VK_NULL_HANDLE;

        VkImageAspectFlags aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;

        uint32_t width = 0;

        uint32_t height = 0;

        uint32_t array_layers = 1;

        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkDescriptorImageInfo descriptor{};

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
            uint32_t array_layers = 1,
            VkPhysicalDevice physical_device = VK_NULL_HANDLE,
            VkDevice device = VK_NULL_HANDLE
        );

        void transition_image_layout(
            VkImageLayout old_layout,
            VkImageLayout new_layout,
            uint32_t layer_index = 0
        );

        void copy_image_data(
            uint32_t width,
            uint32_t height,
            void* pixels,
            uint32_t layer_index = 0
        );

        void make_sampler();

        void update_descriptor();

        void destroy() const;

    };

}