#include <stdexcept>

#include <vulkan/vk_core.h>
#include <vulkan/vk_fences.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

	VkFence _Fence_Pool::_create_item() {

		if (device == VK_NULL_HANDLE) {
			throw std::runtime_error("Vulkan fail to create fence: try to create device first!");
		}

        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VkFence fence = VK_NULL_HANDLE;
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
            VkFence fence = _fences_pool.request_item();
            vkResetFences(device, 1, &fence);
            return fence;
        }

        void release_fence(VkFence fence) {
            _fences_pool.pooling_item(fence);
        } 

    }

}