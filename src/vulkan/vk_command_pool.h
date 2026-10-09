#pragma once
#include <vulkan/vulkan.h>

namespace Vulkan {

	void init_command_pool_threads(VkSurfaceKHR surface, VkPhysicalDevice physical_device, VkDevice device);

	void destroy_command_pool_threads(VkDevice device);

	VkCommandBuffer request_command_buffer(VkDevice device);

	void release_command_buffer(VkDevice device, VkCommandBuffer& command_buffer);

} // namespace Vulkan