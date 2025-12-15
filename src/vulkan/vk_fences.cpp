#include <stdexcept>

#include <vulkan/vk_core.h>
#include <vulkan/vk_fences.h>
#include <vulkan/vk_utils.h>
#include <log.h>

namespace Vulkan {

	VkFence _Fence_Pool::_create_item() {

		if (device == VK_NULL_HANDLE) {
			throw std::runtime_error("Vulkan fail to create fence: try to create device first!");
		}

        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VkFence fence;
        Utils::vk_check_result(
            vkCreateFence(device, &fence_info, nullptr, &fence),
            "",
            "Vulkan fail to create fence!"
        );

        return fence;
	}

    void _Fence_Pool::_delete_item(VkFence& fence) {

        if (fence != VK_NULL_HANDLE) {
            vkDestroyFence(device, fence, nullptr);
        }

    }

    void _update_fences_callback() {

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
            API::release_fence(fence);
        }

        if (_fences_callback.size() <= 0) {
            _global_scheduler->remove_task_by_name(
                Const::VULKAN_FENCE_SCHEDULER_TASK_NAME
            );
        }
          
    }

    namespace Init {

        void _init_fences() {

        }

    }

    namespace Destroy {

        void _destroy_fences() {

            _fences_pool.destroy();

        }

    }

    namespace API {

        VkFence request_fence() {
            return _fences_pool.request_item();
        }

        void release_fence(VkFence fence) {
            _fences_pool.pooling_item(fence);
        }

        template<class F, class... Args>
        auto on_fence_success(VkFence fence, F&& f, Args&&... args)
            -> std::future<typename std::invoke_result<F, Args...>::type> {

            using result_type = typename std::invoke_result<F, Args...>::type;

            auto task = std::make_shared< std::packaged_task<result_type()> >(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );

            if (vkGetFenceStatus(device, fence) == VK_SUCCESS) {
                task();
                release_fence(fence);
            }
            else {
                // add task to list callback when fence excute success
                std::lock_guard<std::mutex> lock(_fences_callback_lock);
                _fences_callback.insert(fence, task);

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