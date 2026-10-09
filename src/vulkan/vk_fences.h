#pragma once

#include <functional>
#include <future>
#include <mutex>
#include <stdexcept>
#include <unordered_map>

#include <concurrent_pool.h>
#include <core.h>
#include <log.h>
#include <vulkan/vk_consts.h>
#include <vulkan/vk_utils.h>
#include <vulkan/vulkan.h>

namespace Vulkan {

	inline std::unordered_map<VkDevice, std::shared_ptr<std::mutex>> fences_callback_locks{};

	inline std::unordered_map<VkDevice, std::unordered_map<VkFence, std::function<void()>>> fences_callbacks{};

	inline std::string get_scheduler_key(VkDevice device) {
		uint64_t device_hash = std::hash<VkDevice>()(device);
		return Const::VULKAN_FENCES_SCHEDULER_TASK_NAME + std::to_string(device_hash);
	}

	struct Fence_Information {
		VkDevice device = VK_NULL_HANDLE;
	};

	class Fence_Pool : public Concurent_Pool<VkFence> {
	  private:
		Fence_Information information{};

		VkFence _create_item() override {

			if (information.device == VK_NULL_HANDLE) {
				throw std::runtime_error("Vulkan fail to create fence: try to create device first!");
			}

			VkFenceCreateInfo fence_info{};
			fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			VkFence fence = VK_NULL_HANDLE;
			Utils::vk_check_result(
				vkCreateFence(information.device, &fence_info, nullptr, &fence), "", "Vulkan fail to create fence!"
			);

			return fence;
		}

		void _delete_item(VkFence& fence) override {
			if (fence != VK_NULL_HANDLE) {
				vkDestroyFence(information.device, fence, nullptr);
			}
		}

	  public:
		Fence_Pool() {}

		Fence_Pool(Fence_Information information) { this->information = information; };
	};

	inline std::unordered_map<VkDevice, std::shared_ptr<Fence_Pool>> fences_pools{};

	inline void update_fences_callback(VkDevice device) {
		std::unique_lock<std::mutex> lock(*fences_callback_locks[device]);
		std::vector<VkFence> fences_need_remove;

		for (auto& [fence, callback] : fences_callbacks[device]) {
			if (vkGetFenceStatus(device, fence) == VK_SUCCESS) {
				Core::global_thread_pool->enqueue(std::move(callback));
				fences_need_remove.push_back(fence);
			}
		}

		for (auto& fence : fences_need_remove) {
			fences_callbacks[device].erase(fence);
		}

		if (fences_callbacks[device].size() <= 0) {
			Core::global_scheduler->pause_scheduler_task(get_scheduler_key(device));
		}
	}

	inline void init_fences(VkDevice device) {
		fences_pools[device] = std::make_shared<Fence_Pool>(Fence_Information{device});
		fences_callback_locks[device] = std::make_shared<std::mutex>();
		fences_callbacks[device] = {};
		Core::global_scheduler->schedule(
			[device](long long dt) { update_fences_callback(device); }, get_scheduler_key(device)
		);
	}

	inline void destroy_fences(VkDevice device) {
		if (Core::global_scheduler->is_contain_task(get_scheduler_key(device))) {
			Core::global_scheduler->remove_task_by_name(get_scheduler_key(device));
		}
		fences_pools[device]->destroy();
	}

	inline VkFence request_fence(VkDevice device, bool signaled = false) {
		if (fences_pools.find(device) == fences_pools.end()) {
			throw std::runtime_error(
				"Fail to request fence try to init fences with this device " + std::to_string((uint64_t)device) +
				" first!"
			);
		}
		VkFence fence = fences_pools[device]->request_item();
		if (!signaled && vkGetFenceStatus(device, fence) == VK_SUCCESS) {
			vkResetFences(device, 1, &fence);
		}
		return fence;
	}

	inline void release_fence(VkDevice device, VkFence fence) {
		if (fences_pools.find(device) == fences_pools.end()) {
			throw std::runtime_error(
				"Fail to release fence try to init fences with this device " + std::to_string((uint64_t)device) +
				" first!"
			);
		}
		vkResetFences(device, 1, &fence);
		fences_pools[device]->pooling_item(fence);
	}

	template <class F, class... Args>
	inline auto on_fence_success(VkDevice device, VkFence fence, F&& f, Args&&... args)
		-> std::future<typename std::invoke_result<F, Args...>::type> {

		using result_type = typename std::invoke_result<F, Args...>::type;

		auto task = std::make_shared<std::packaged_task<result_type()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);

		if (vkGetFenceStatus(device, fence) == VK_SUCCESS) {
			(*task)();
		} else {
			{
				if (fences_callback_locks.find(device) == fences_callback_locks.end()) {
					throw std::runtime_error(
						"Fail to release fence try to init fences with this device " +
						std::to_string((uint64_t)device) + " first!"
					);
				}
				// add task to list callback when fence excute success
				std::unique_lock<std::mutex> lock(*fences_callback_locks[device]);
				fences_callbacks[device].emplace(fence, [task]() { (*task)(); });
				Core::global_scheduler->unpause_scheduler_task(get_scheduler_key(device));
			}
		}

		return task->get_future();
	}

} // namespace Vulkan