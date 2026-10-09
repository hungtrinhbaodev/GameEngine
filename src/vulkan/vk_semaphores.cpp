#include <concurrent_pool.h>
#include <map>
#include <vulkan/vk_semaphores.h>

namespace Vulkan {

	struct Semaphores_Information {
		VkDevice device = VK_NULL_HANDLE;
	};

	class Semaphore_Pool : public Concurent_Pool<VkSemaphore> {
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
		Semaphore_Pool(Semaphores_Information information) { this->information = information; }
	};

	std::unordered_map<VkDevice, std::shared_ptr<Semaphore_Pool>> semaphore_pools{};

	void init_semaphores(VkDevice device) {
		semaphore_pools[device] = std::make_shared<Semaphore_Pool>(Semaphores_Information{device});
	}

	VkSemaphore request_semaphore(VkDevice device) {
		if (semaphore_pools.find(device) == semaphore_pools.end()) {
			throw std::runtime_error(
				"Fail to request semaphore try to init semaphores with this device " +
				std::to_string((uint64_t)device) + " first!"
			);
		}
		return semaphore_pools[device]->request_item();
	}

	void release_semaphore(VkDevice device, VkSemaphore semaphore) {
		if (semaphore_pools.find(device) == semaphore_pools.end()) {
			throw std::runtime_error(
				"Fail to release semaphore try to init semaphores with this device " +
				std::to_string((uint64_t)device) + " first!"
			);
		}
		semaphore_pools[device]->pooling_item(semaphore);
	}

	void destroy_semaphores(VkDevice device) {
		if (semaphore_pools.find(device) == semaphore_pools.end()) {
			return;
		}
		semaphore_pools[device]->destroy();
	}

} // namespace Vulkan