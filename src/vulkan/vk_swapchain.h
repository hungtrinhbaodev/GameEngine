#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

namespace Vulkan {

	VkSurfaceFormatKHR _choose_swapchain_format(const std::vector<VkSurfaceFormatKHR>& available_formats);

	VkPresentModeKHR _choose_swapchain_present_mode(const std::vector<VkPresentModeKHR>& available_presents);

	VkExtent2D _choose_swapchain_extent(const VkSurfaceCapabilitiesKHR& capabilites, GLFWwindow* window);

	void _create_swapchain_image_views(
		const std::vector<VkImage>& swapchain_images, std::vector<VkImageView>& swapchain_image_views,
		const VkFormat& format, VkDevice device
	);

	namespace Init {
		void _init_swapchain(
			GLFWwindow* window, VkSwapchainKHR& swapchain, VkFormat& swapchain_format, VkExtent2D& swapchain_extent,
			std::vector<VkImage>& swapchain_images, std::vector<VkImageView>& swapchain_image_views,
			VkSurfaceKHR surface, VkPhysicalDevice physical_device, VkDevice device
		);
	}

	namespace Process {
		void _recreate_swapchain(
			GLFWwindow* window, VkSwapchainKHR& swapchain, VkFormat& swapchain_format, VkExtent2D& swapchain_extent,
			std::vector<VkImage>& swapchain_images, std::vector<VkImageView>& swapchain_image_views,
			VkSurfaceKHR surface, VkPhysicalDevice physical_device, VkDevice device
		);
	}

	namespace Destroy {
		void _destroy_swapchain(
			VkSwapchainKHR swapchain, const std::vector<VkImageView>& swapchain_image_views, VkDevice device
		);
	}

} // namespace Vulkan