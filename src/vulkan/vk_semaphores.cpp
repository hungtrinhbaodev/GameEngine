#include <concurrent_pool.h>
#include <map>
#include <vulkan/vk_semaphores.h>

namespace Vulkan {

	struct Semaphores_Information {
		VkDevice device = VK_NULL_HANDLE;
	};

	class _Semaphore_Pool : public Concurent_Pool<VkSemaphore> {
	  private:
		Semaphores_Information information{};

		VkSemaphore _create_item() override {
			VkSemaphoreCreateInfo semaphore_info{};
			semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			VkSemaphore vk_semaphore;
			vkCreateSemaphore(information.device, &semaphore_info, nullptr, &vk_semaphore);
			return vk_semaphore;
		}

		void _delete_item(VkSemaphore& item) override { vkDestroySemaphore(information.device, item, nullptr); }

	  public:
		_Semaphore_Pool(Semaphores_Information information) { this->information = information; }
	};

	std::unordered_map<VkDevice, std::shared_ptr<_Semaphore_Pool>> _semaphore_pools{};

	void init_semaphores(VkDevice device) {
		_semaphore_pools[device] = std::make_shared<_Semaphore_Pool>(Semaphores_Information{device});
	}

	namespace API {
		VkSemaphore request_semaphore(VkDevice device) {
			if (_semaphore_pools.find(device) == _semaphore_pools.end()) {
				throw std::runtime_error(
					"Fail to request semaphore try to init semaphores with this device " +
					std::to_string((uint64_t)device) + " first!"
				);
			}
			return _semaphore_pools[device]->request_item();
		}

		void release_semaphore(VkDevice device, VkSemaphore semaphore) {
			if (_semaphore_pools.find(device) == _semaphore_pools.end()) {
				throw std::runtime_error(
					"Fail to release semaphore try to init semaphores with this device " +
					std::to_string((uint64_t)device) + " first!"
				);
			}
			_semaphore_pools[device]->pooling_item(semaphore);
		}
	} // namespace API

	namespace Destroy {
		void _destroy_semaphores(VkDevice device) {
			if (_semaphore_pools.find(device) == _semaphore_pools.end()) {
				return;
			}
			_semaphore_pools[device]->destroy();
		}
	} // namespace Destroy

} // namespace Vulkan