#pragma once
#include <vulkan/vulkan.h>

#include <graphic/vulkan_implementation/vulkan_constants.h>
#include <graphic/vulkan_implementation/vulkan_utility.h>
#include <graphic/vulkan_implementation/vulkan_queues.h>
#include <graphic/vulkan_implementation/vulkan_buffer.h>
#include <graphic/vulkan_implementation/vulkan_command_pool.h>


namespace Graphic {

    using Image_Callback = std::function<void(void*)>;

    class Vulkan_Image {

        private:

        VkImage _vk_image = VK_NULL_HANDLE;

        VkDeviceMemory _vk_image_memory = VK_NULL_HANDLE;

        VkImageView _vk_imageview = VK_NULL_HANDLE;

        public:

        void make(
            uint32_t width,
            uint32_t height,
            VkFormat format,
            VkImageTiling tiling,
            VkImageUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkDevice vk_device = VK_NULL_HANDLE,
            VkPhysicalDevice vk_physical_device = VK_NULL_HANDLE
        );

        void transition_image_layout(
            VkFormat format,
            VkImageLayout old_layout,
            VkImageLayout new_layout,
            void* user_data = nullptr,
            Image_Callback callback = nullptr,
            Vulkan_Command_Pool* vk_command_pool = nullptr,
            Vulkan_Queues* vk_queues = nullptr
        );

        void copy_buffer_to_image(
            int width,
            int height,
            Vulkan_Buffer* staging_buffer,
            void* user_data = nullptr,
            Image_Callback callback = nullptr,
            Vulkan_Command_Pool* vk_command_pool = nullptr,
            Vulkan_Queues* vk_queues = nullptr
        );

        VkImage get_image();

        VkImageView get_imageview();

        void destroy(VkDevice vk_device);
    };

}