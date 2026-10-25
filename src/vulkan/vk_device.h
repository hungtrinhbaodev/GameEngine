#pragma once
#include <vulkan/vulkan.h>

namespace Vulkan {

	void init_device(VkDevice& device, VkSurfaceKHR surface, VkPhysicalDevice physical_device);

	void destroy_device(VkDevice device);

	/**
	 * @Note: VK_EXT_memory_budget is optional, it is only enabled when the device supports it
	 * and the instance api is at least 1.1 (vkGetPhysicalDeviceMemoryProperties2 is core there).
	 */
	bool is_memory_budget_enabled();

} // namespace Vulkan