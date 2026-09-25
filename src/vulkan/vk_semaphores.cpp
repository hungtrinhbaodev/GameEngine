#include <vulkan/vk_core.h>
#include <vulkan/vk_semaphores.h>

namespace Vulkan {

	VkSemaphore _Semaphore_Pool::_create_item() {
		VkSemaphoreCreateInfo semaphore_info{};
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VkSemaphore vk_semaphore;
		vkCreateSemaphore(device, &semaphore_info, nullptr, &vk_semaphore);
		return vk_semaphore;
	}

	void _Semaphore_Pool::_delete_item(VkSemaphore& item) {
		vkDestroySemaphore(device, item, nullptr);
	}

	_Semaphore_Pool _semaphore_pool;

	namespace API {
		VkSemaphore request_semaphore() {
			return _semaphore_pool.request_item();
		}

		void release_semaphore(VkSemaphore semaphore) {
			_semaphore_pool.pooling_item(semaphore);
		}
	} // namespace API

	namespace Destroy {
		void _destroy_semaphores() {
			_semaphore_pool.destroy();
		}
	} // namespace Destroy

} // namespace Vulkan