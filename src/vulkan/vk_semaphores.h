#pragma once
#include <vulkan/vulkan.h>
#include <concurrent_pool.h>

namespace Vulkan {
	class _Semaphore_Pool : public Concurent_Pool<VkSemaphore> {

		VkSemaphore _create_item() override;

		void _delete_item(VkSemaphore& item) override;
	};

	extern _Semaphore_Pool semaphore_pool;

	namespace API {

		VkSemaphore request_semaphore();

		void release_semaphore(VkSemaphore semaphore);

	} // namespace API

	namespace Destroy {
		void _destroy_semaphores();
	}
} // namespace Vulkan