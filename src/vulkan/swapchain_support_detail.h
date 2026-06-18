#pragma once

#include <vector>

#include <vulkan/vulkan.h>

namespace Vulkan {

	struct Swapchain_Support_Detail {

		VkSurfaceCapabilitiesKHR capabilities;

		std::vector<VkSurfaceFormatKHR> formats;

		std::vector<VkPresentModeKHR> present_modes;

		void log_info() const;
	};

} // namespace Vulkan