#pragma once
#include <vulkan/vulkan.h>

#include <graphic/vulkan_implementation/vulkan_constants.h>
#include <graphic/vulkan_implementation/vulkan_utility.h>
#include <graphic/vulkan_implementation/vulkan_queues.h>
#include <graphic/vulkan_implementation/vulkan_buffer.h>
#include <graphic/vulkan_implementation/vulkan_command_pool.h>


namespace Graphic {

    class Vulkan_Image {

        private:

        VkImage _vk_image = VK_NULL_HANDLE;

        VkDeviceMemory _vk_image_memory = VK_NULL_HANDLE;

        VkImageView _vk_imageview = VK_NULL_HANDLE;

        VkImageAspectFlags _vk_aspect_flags;

        public:

        void make(
            uint32_t width,
            uint32_t height,
            VkFormat format,
            VkImageTiling tiling,
            VkImageUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkImageAspectFlags vk_aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT
        );

        Vulkan_Commands_Record_Data make_transition_record_data(
            VkFormat format,
            VkImageLayout old_layout,
            VkImageLayout new_layout,
            Vulkan_Command_Callback callback = nullptr
        );

        Vulkan_Commands_Record_Data make_copy_to_image_record_data(
            int width,
            int height,
            void* pixels,
            Vulkan_Command_Callback callback = nullptr
        );

        VkImage get_image();

        VkImageView get_imageview();

        void destroy(VkDevice vk_device);
    };

}