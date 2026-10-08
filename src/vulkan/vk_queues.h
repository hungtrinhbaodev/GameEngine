#pragma once
#include <vulkan/vulkan.h>

namespace Vulkan {

	namespace Init {
		void _init_queues(
			VkQueue& graphics_queue, VkQueue& present_queue, VkSurfaceKHR surface, VkPhysicalDevice physical_device,
			VkDevice device
		);
	}

	namespace API {
		void submit(const VkSubmitInfo& submit_info, VkQueue submit_queue, VkFence fence = VK_NULL_HANDLE);

		VkResult submit_present(const VkPresentInfoKHR& present_info, VkQueue present_queue);
	} // namespace API

} // namespace Vulkan