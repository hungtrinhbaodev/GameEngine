#pragma once
#include <vulkan/vulkan.h>

#include <vector>

#include <utility/log_utils.h>

namespace Graphic {

    class Vulkan_Swapchain_Support_Detail {

        public:

        VkSurfaceCapabilitiesKHR capabilities;

        std::vector<VkSurfaceFormatKHR> formats;

        std::vector<VkPresentModeKHR> present_modes;

        void log_info();
    };

}