#pragma once
#include <vulkan/vulkan.h>

namespace Vulkan {

	void init_semaphores(VkDevice device);

	namespace API {

		VkSemaphore request_semaphore(VkDevice device);

		void release_semaphore(VkDevice device, VkSemaphore semaphore);

	} // namespace API

	namespace Destroy {
		void _destroy_semaphores(VkDevice device);
	}
} // namespace Vulkan