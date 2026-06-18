#pragma once
#include <mutex>

namespace Vulkan {

	namespace Init {

		void _init_queues();

	}

	namespace API {

		void submit(const VkSubmitInfo& submit_info, VkFence fence = VK_NULL_HANDLE,
					VkQueue submit_queue = Vulkan::graphics_queue);

	}

} // namespace Vulkan