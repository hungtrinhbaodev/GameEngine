#pragma once
#include <vulkan/vulkan.h>

#include <graphic/vulkan_implementation/vulkan_constants.h>
#include <graphic/vulkan_implementation/vulkan_utility.h>
#include <utility/log_utils.h>

namespace Graphic {

    class Vulkan_Frame_Buffers {

        private:

        std::vector<VkFramebuffer> _vk_frame_buffers;

        public:

        void init(
            VkDevice vk_device,
            VkRenderPass vk_render_pass,
            const std::vector<VkImageView>& vk_swapchain_imageviews,
            VkExtent2D vk_swapchain_extent
        );

        const std::vector<VkFramebuffer>& get();

        void destroy(VkDevice vk_device);
    };
}