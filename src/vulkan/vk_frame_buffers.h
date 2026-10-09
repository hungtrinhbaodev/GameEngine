#pragma once
#include <vulkan/vk_image.h>
#include <vulkan/vulkan.h>

namespace Vulkan {

	namespace Init {
		void _init_frame_buffers(
			std::vector<VkFramebuffer>& frame_buffers, VkRenderPass render_pass, VkDevice device,
			const std::vector<VkImageView>& swapchain_image_views, Image depth_image, VkExtent2D swapchain_extent
		);
	}

	namespace Process {
		void _recreate_frame_buffers(
			std::vector<VkFramebuffer>& frame_buffers, VkRenderPass render_pass, VkDevice device,
			const std::vector<VkImageView>& swapchain_image_views, Image depth_image, VkExtent2D swapchain_extent
		);
	}

	namespace Destroy {
		void _destroy_frame_buffers(const std::vector<VkFramebuffer>& frame_buffers, VkDevice device);
	}

} // namespace Vulkan