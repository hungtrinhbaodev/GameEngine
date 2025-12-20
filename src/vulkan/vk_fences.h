#pragma once

#include <unordered_map>
#include <mutex>
#include <future>
#include <functional>

#include <vulkan/vulkan.h>
#include <concurrent_pool.h>
#include <vulkan/vk_consts.h>
#include <log.h>

namespace Vulkan {

	class _Fence_Pool : public Concurent_Pool<VkFence> {

		VkFence _create_item() override;

		void _delete_item(VkFence& item) override;

	};

	namespace {

		inline _Fence_Pool _fences_pool;

		inline std::mutex _fences_callback_lock;

		inline std::unordered_map<VkFence, std::function<void()>> _fences_callback;

	}

	inline void _update_fences_callback() {

        std::lock_guard<std::mutex> lock(_fences_callback_lock);

        std::vector<VkFence> fences_need_remove;
        for (auto& [fence, callback] : _fences_callback) {
            if (vkGetFenceStatus(device, fence) == VK_SUCCESS) {
                _global_thread_pool->enqueue(std::move(callback));
                fences_need_remove.push_back(fence);
            }
        }

        for (auto& fence : fences_need_remove) {
            _fences_callback.erase(fence);
        }
          
    }

	namespace Init {

		void _init_fences();

	}

	namespace Destroy {

		void _destroy_fences();

	}

	namespace API {

		VkFence request_fence();

		void release_fence(VkFence fence);

		template<class F, class... Args>
		inline auto on_fence_success(VkFence fence, F&& f, Args&&... args)
			->std::future<typename std::invoke_result<F, Args...>::type> {

            using result_type = typename std::invoke_result<F, Args...>::type;

            auto task = std::make_shared<std::packaged_task<result_type()> >(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );

            if (vkGetFenceStatus(device, fence) == VK_SUCCESS) {
                (*task)();
            }
            else {
                // add task to list callback when fence excute success
                std::lock_guard<std::mutex> lock(_fences_callback_lock);
                _fences_callback.emplace(fence, [task] () {
					(*task)();
				});

                Log::log_info("on_fence_success 1", _fences_callback.size());
                if (!_global_scheduler->is_contain_task(Const::VULKAN_FENCE_SCHEDULER_TASK_NAME)) {
                    _global_scheduler->schedule([](long long) {
                        _update_fences_callback();
                    }, Const::VULKAN_FENCE_SCHEDULER_TASK_NAME);
                }
            }

            return task->get_future();
        }

	}
}