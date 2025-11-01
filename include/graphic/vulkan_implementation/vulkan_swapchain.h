#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <vector>

#include <graphic/vulkan_implementation/vulkan_constants.h>
#include <graphic/vulkan_implementation/vulkan_utility.h>
#include <graphic/vulkan_implementation/vulkan_swapchain_support_detail.h>
#include <graphic/vulkan_implementation/vulkan_queue_family_indices.h>
#include <utility/log_utils.h>

namespace Graphic {

    class Vulkan_Swapchain {

        private:

        VkFormat _vk_swapchain_format;
        
        VkExtent2D _vk_swapchain_extent;

        VkSwapchainKHR _vk_swapchain = VK_NULL_HANDLE;

        std::vector<VkImage> _vk_swapchain_images;

        std::vector<VkImageView> _vk_swapchain_imageviews;

        void _create_swapchain_image_views(
            const std::vector<VkImage>& swapchain_images,
            const VkFormat& format,
            VkDevice vk_device
        );

        public:

        VkSurfaceFormatKHR choose_swapchain_format(const std::vector<VkSurfaceFormatKHR>& available_formats);
        
        VkPresentModeKHR choose_swapchain_present_mode(const std::vector<VkPresentModeKHR>& available_presents);

        VkExtent2D choose_swapchain_extent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);

        void init(
            VkPhysicalDevice vk_physical_device,
            VkSurfaceKHR vk_surface,
            VkDevice vk_device,
            GLFWwindow* window
        );

        void recreate_swapchain(
            VkPhysicalDevice vk_physical_device,
            VkSurfaceKHR vk_surface,
            VkDevice vk_device,
            GLFWwindow* window
        );

        VkSwapchainKHR get();

        VkFormat get_format();

        VkExtent2D get_extent();

        std::vector<VkImageView>& get_imageviews();

        void destroy(VkDevice vk_device);
    };
}