#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

namespace Vulkan {

	void init_swapchain(
		GLFWwindow* window, VkSwapchainKHR& swapchain, VkFormat& swapchain_format, VkExtent2D& swapchain_extent,
		std::vector<VkImage>& swapchain_images, std::vector<VkImageView>& swapchain_image_views, VkSurfaceKHR surface,
		VkPhysicalDevice physical_device, VkDevice device
	);

	void recreate_swapchain(
		GLFWwindow* window, VkSwapchainKHR& swapchain, VkFormat& swapchain_format, VkExtent2D& swapchain_extent,
		std::vector<VkImage>& swapchain_images, std::vector<VkImageView>& swapchain_image_views, VkSurfaceKHR surface,
		VkPhysicalDevice physical_device, VkDevice device
	);

	void destroy_swapchain(
		VkSwapchainKHR swapchain, const std::vector<VkImageView>& swapchain_image_views, VkDevice device
	);

} // namespace Vulkan