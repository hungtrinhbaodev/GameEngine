#pragma once
#include <vulkan/vk_image.h>

namespace Vulkan {

	namespace Init {
		void _init_depth_image(
			Image& depth_image, VkPhysicalDevice physical_device, VkDevice device, VkExtent2D swapchain_extent
		);
	}

	namespace Process {
		void _recreate_depth_image(
			Image& depth_image, VkPhysicalDevice physical_device, VkDevice device, VkExtent2D swapchain_extent
		);
	}

	namespace Destroy {
		void _destroy_depth_image(Image depth_image);
	}

} // namespace Vulkan