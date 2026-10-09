#pragma once
#include <vulkan/vulkan.h>

namespace Vulkan {

	namespace Init {
		void _init_render_pass(
			VkRenderPass& render_pass, VkPhysicalDevice physical_device, VkDevice device, VkFormat swapchain_format
		);
	}

	namespace Destroy {
		void _destroy_render_pass(VkRenderPass render_pass, VkDevice device);
	}

} // namespace Vulkan