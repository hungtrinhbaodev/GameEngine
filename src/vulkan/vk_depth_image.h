#pragma once
#include <vulkan/vk_image.h>

namespace Vulkan {

	void init_depth_image(
		Image& depth_image, VkPhysicalDevice physical_device, VkDevice device, VkExtent2D swapchain_extent
	);

	void recreate_depth_image(
		Image& depth_image, VkPhysicalDevice physical_device, VkDevice device, VkExtent2D swapchain_extent
	);

	void destroy_depth_image(Image depth_image);

} // namespace Vulkan