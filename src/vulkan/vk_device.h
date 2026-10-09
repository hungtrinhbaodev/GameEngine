#pragma once
#include <vulkan/vulkan.h>

namespace Vulkan {

	void init_device(VkDevice& device, VkSurfaceKHR surface, VkPhysicalDevice physical_device);

	void destroy_device(VkDevice device);

} // namespace Vulkan