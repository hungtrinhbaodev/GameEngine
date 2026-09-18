#pragma once
#include <mutex>
#include <vulkan/vulkan.h>

namespace Vulkan {

	extern std::mutex _submit_mutex;

	namespace Init {
		void _init_queues();
	}

	namespace API {
		void submit(
			const VkSubmitInfo& submit_info, VkFence fence = VK_NULL_HANDLE, VkQueue submit_queue = VK_NULL_HANDLE
		);

		void submit_present(const VkPresentInfoKHR& present_info);
	} // namespace API

} // namespace Vulkan