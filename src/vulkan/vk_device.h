#pragma once
#include <vulkan/vulkan.h>

namespace Vulkan {

	namespace Init {
		void _init_device(VkDevice& device, VkSurfaceKHR surface, VkPhysicalDevice physical_device);
	}

	namespace Destroy {
		void _destroy_device(VkDevice device);
	}

} // namespace Vulkan