#pragma once

#include <vulkan/vulkan.h>

namespace Vulkan {


    VkSurfaceFormatKHR _choose_swapchain_format(const std::vector<VkSurfaceFormatKHR>& available_formats);

    VkPresentModeKHR _choose_swapchain_present_mode(const std::vector<VkPresentModeKHR>& available_presents);

    VkExtent2D _choose_swapchain_extent(const VkSurfaceCapabilitiesKHR& capabilites, GLFWwindow* window);

    void _create_swapchain_image_views(
        const std::vector<VkImage>& swapchain_images, 
        const VkFormat& format,
        VkDevice vk_device
    );


    namespace Init {

        void _init_swapchain();

    }

    namespace Destroy {

        void _destroy_swapchain();

    }

}