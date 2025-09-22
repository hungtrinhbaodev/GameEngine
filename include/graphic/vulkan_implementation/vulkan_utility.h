#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <string>
#include <stdexcept>

#include <graphic/vulkan_implementation/vulkan_constants.h>
#include <graphic/vulkan_implementation/vulkan_queue_family_indices.h>
#include <graphic/vulkan_implementation/vulkan_swapchain_support_detail.h>

namespace Graphic {

    class Vulkan_Utility {

        public:

        static void vk_check_action(VkResult result, std::string crash_message);

        static std::vector<const char*> query_instance_extensions();

        static std::vector<VkLayerProperties> query_instance_layer_propeties();

        static std::vector<const char*> query_instance_layer_enabled();

        static bool is_validation_layer_enabled();

        static std::vector<const char *> query_physical_device_support_required_extensions(VkPhysicalDevice vk_physical_device); 

        static Vulkan_Queue_Family_Indices query_suitable_queue_family_indices(VkPhysicalDevice vk_physical_device, VkSurfaceKHR vk_surface);

        static Vulkan_Swapchain_Support_Detail query_swapchain_support_detail(VkPhysicalDevice vk_physical_device, VkSurfaceKHR vk_surface);

        static bool is_suitable_physical_device(VkPhysicalDevice vk_physical_device, VkSurfaceKHR vk_surface);
    };
}