#include <vulkan/vulkan.h>

#include <vector>

#include <graphic/vulkan_implementation/vulkan_constants.h>
#include <graphic/vulkan_implementation/vulkan_utility.h>

namespace Graphic {

    class Vulkan_Render_Pass {

        private:

        VkRenderPass _vk_render_pass = VK_NULL_HANDLE;

        public:

        void init(
            VkDevice vk_device,
            VkPhysicalDevice vk_physical_device,
            VkFormat vk_swapchain_format
        );

        VkRenderPass get();

        void destroy(VkDevice vk_device);
    };
}
