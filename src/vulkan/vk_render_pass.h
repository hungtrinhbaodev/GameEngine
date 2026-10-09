#pragma once
#include <vulkan/vulkan.h>

namespace Vulkan {

	void init_render_pass(
		VkRenderPass& render_pass, VkPhysicalDevice physical_device, VkDevice device, VkFormat swapchain_format
	);

	void destroy_render_pass(VkRenderPass render_pass, VkDevice device);

} // namespace Vulkan