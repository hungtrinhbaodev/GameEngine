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

	inline VkDevice fence_device = VK_NULL_HANDLE;

	inline std::mutex fences_callback_lock{};

	inline std::unordered_map<VkFence, std::function<void()>> fences_callback{};

	class _Fence_Pool : public Concurent_Pool<VkFence> {
		VkFence _create_item() override {

			if (fence_device == VK_NULL_HANDLE) {
				throw std::runtime_error("Vulkan fail to create fence: try to create device first!");
			}

			VkFenceCreateInfo fence_info{};
			fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			VkFence fence = VK_NULL_HANDLE;
			Utils::vk_check_result(
				vkCreateFence(fence_device, &fence_info, nullptr, &fence), "", "Vulkan fail to create fence!"
			);

			return fence;
		}

		void _delete_item(VkFence& fence) override {
			if (fence != VK_NULL_HANDLE) {
				vkDestroyFence(fence_device, fence, nullptr);
			}
		}
	};

	inline _Fence_Pool fences_pool{};

	inline void _update_fences_callback() {
		std::unique_lock<std::mutex> lock(fences_callback_lock);
		std::vector<VkFence> fences_need_remove;

		for (auto& [fence, callback] : fences_callback) {
			if (vkGetFenceStatus(fence_device, fence) == VK_SUCCESS) {
				Core::global_thread_pool->enqueue(std::move(callback));
				fences_need_remove.push_back(fence);
			}
		}

		for (auto& fence : fences_need_remove) {
			fences_callback.erase(fence);
		}

		if (fences_callback.size() <= 0) {
			Core::global_scheduler->pause_scheduler_task(Const::VULKAN_FENCES_SCHEDULER_TASK_NAME);
		}
	}

	namespace Init {
		inline void _init_fences(VkDevice device) {
			fence_device = device;
			Core::global_scheduler->schedule(
				[](long long dt) { _update_fences_callback(); }, Const::VULKAN_FENCES_SCHEDULER_TASK_NAME
			);
		}

	} // namespace Init

	namespace Destroy {
		inline void _destroy_fences() {
			if (Core::global_scheduler->is_contain_task(Const::VULKAN_FENCES_SCHEDULER_TASK_NAME)) {
				Core::global_scheduler->remove_task_by_name(Const::VULKAN_FENCES_SCHEDULER_TASK_NAME);
			}
			fences_pool.destroy();
		}

	} // namespace Destroy

	namespace API {
		inline VkFence request_fence(bool signaled = false) {
			VkFence fence = fences_pool.request_item();
			if (!signaled && vkGetFenceStatus(fence_device, fence) == VK_SUCCESS) {
				vkResetFences(fence_device, 1, &fence);
			}
			return fence;
		}

		inline void release_fence(VkFence fence) {
			vkResetFences(fence_device, 1, &fence);
			fences_pool.pooling_item(fence);
		}

		template <class F, class... Args>
		inline auto on_fence_success(VkFence fence, F&& f, Args&&... args)
			-> std::future<typename std::invoke_result<F, Args...>::type> {

			using result_type = typename std::invoke_result<F, Args...>::type;

			auto task = std::make_shared<std::packaged_task<result_type()>>(
				std::bind(std::forward<F>(f), std::forward<Args>(args)...)
			);

			if (vkGetFenceStatus(fence_device, fence) == VK_SUCCESS) {
				(*task)();
			} else {
				{
					// add task to list callback when fence excute success
					std::unique_lock<std::mutex> lock(fences_callback_lock);
					fences_callback.emplace(fence, [task]() { (*task)(); });
					Core::global_scheduler->unpause_scheduler_task(Const::VULKAN_FENCES_SCHEDULER_TASK_NAME);
				}
			}

			return task->get_future();
		}
	} // namespace API
} // namespace Vulkan