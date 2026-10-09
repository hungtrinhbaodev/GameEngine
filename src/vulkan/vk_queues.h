#pragma once
#include <vulkan/vulkan.h>

namespace Vulkan {

	void init_queues(
		VkQueue& graphics_queue, VkQueue& present_queue, VkSurfaceKHR surface, VkPhysicalDevice physical_device,
		VkDevice device
	);

	void submit(const VkSubmitInfo& submit_info, VkDevice device, VkFence fence = VK_NULL_HANDLE);

	VkResult submit_present(const VkPresentInfoKHR& present_info, VkDevice device);

} // namespace Vulkan