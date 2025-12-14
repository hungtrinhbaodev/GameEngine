#pragma once

#include <unordered_map>
#include <mutex>
#include <future>
#include <functional>

#include <vulkan/vulkan.h>

#include <concurrent_pool.h>

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

	void _update_fences_callback();

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
		auto on_fence_success(VkFence fence, F&& f, Args&&... args)
			->std::future<typename std::invoke_result<F, Args...>::type>;

	}
}