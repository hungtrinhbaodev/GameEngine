#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <string>
#include <stdexcept>

#include <core/resource.hpp>
#include <graphic/vulkan_implementation/vulkan_constants.h>
#include <graphic/vulkan_implementation/vulkan_queue_family_indices.h>
#include <graphic/vulkan_implementation/vulkan_swapchain_support_detail.h>
#include <graphic/vulkan_implementation/vulkan_queues.h>
#include <graphic/vulkan_implementation/vulkan_command_pool.h>

namespace Graphic {

    struct Vulkan_Device_Default_Data {
        VkDevice vk_device ;
        VkPhysicalDevice vk_physical_device;
    };

    struct Vulkan_Submit_Default_Data {
        Vulkan_Queues* queues;
        Vulkan_Command_Pool* command_pool;
    };

    class Vulkan_Utility {

        public:

        static void vk_check_action(VkResult result, std::string crash_message);

        static std::vector<const char*> query_instance_extensions();

        static std::vector<VkLayerProperties> query_instance_layer_propeties();

        static std::vector<const char*> query_instance_layer_enabled();

        static bool is_validation_layer_enabled();

        static std::vector<const char*> query_physical_device_support_required_extensions(VkPhysicalDevice vk_physical_device); 

        static Vulkan_Queue_Family_Indices query_suitable_queue_family_indices(VkPhysicalDevice vk_physical_device, VkSurfaceKHR vk_surface);

        static Vulkan_Swapchain_Support_Detail query_swapchain_support_detail(VkPhysicalDevice vk_physical_device, VkSurfaceKHR vk_surface);

        static bool is_suitable_physical_device(VkPhysicalDevice vk_physical_device, VkSurfaceKHR vk_surface);

        static std::vector<const char*> query_physical_device_layers_enabled(VkPhysicalDevice vk_physical_device);

        static VkImageView create_imageview_from_image(
            VkImage vk_image, 
            const VkFormat& vk_format, 
            VkImageAspectFlags aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT,
            VkDevice vk_device = VK_NULL_HANDLE
        );

        static uint32_t find_buffer_memory_type_index(
            uint32_t type_filter,
            VkMemoryAllocateFlags properties,
            VkPhysicalDevice vk_physical_device = VK_NULL_HANDLE
        );

        static Vulkan_Device_Default_Data get_or_default_device(VkDevice vk_device = VK_NULL_HANDLE, VkPhysicalDevice vk_physical_device = VK_NULL_HANDLE);

        static Vulkan_Submit_Default_Data get_or_default_submit(Vulkan_Queues* wp_queues = nullptr, Vulkan_Command_Pool* wp_command_pool= nullptr);

        static Vulkan_Commands_Mode get_command_mode_by_load_resource_mode(Core::Resource_Load_Mode resource_mode);

        static VkFormat find_supported_format(
            const std::vector<VkFormat>& candidates, 
            VkImageTiling tiling, 
            VkFormatFeatureFlags features,
            VkPhysicalDevice vk_physical_device
        );

        static VkFormat find_depth_format(
            VkPhysicalDevice vk_physical_device
        );

    };
}