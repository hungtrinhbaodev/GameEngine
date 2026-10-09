#pragma once
#include <vulkan/vulkan.h>

namespace Vulkan {

	void init_semaphores(VkDevice device);

	VkSemaphore request_semaphore(VkDevice device);

	void release_semaphore(VkDevice device, VkSemaphore semaphore);

	void destroy_semaphores(VkDevice device);
} // namespace Vulkan