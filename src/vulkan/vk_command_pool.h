#pragma once
#include <vulkan/vulkan.h>

namespace Vulkan {

	namespace Init {
		void _init_command_pool_threads(VkSurfaceKHR surface, VkPhysicalDevice physical_device, VkDevice device);
	}

	namespace Destroy {
		void _destroy_command_pool_threads(VkDevice device);
	}

	namespace API {
		VkCommandBuffer request_command_buffer(VkDevice device);
		void release_command_buffer(VkDevice device, VkCommandBuffer& command_buffer);
	} // namespace API

} // namespace Vulkan