#pragma once
#include <vulkan/vulkan.h>

namespace Graphic {

    template <typename Builder>
    class Vulkan_Builder {
        
        protected:

        VkInstance _vk_instance;

        VkSurfaceKHR _vk_surface;

        VkPhysicalDevice _vk_physical_device;

        VkDevice _vk_device;

        VkRenderPass _vk_render_pass;

        public:

        Builder& add_vk_instance(VkInstance vk_instance) {
            _vk_instance = vk_instance;
            return static_cast<Builder&>(*this);
        }

        Builder& add_vk_device(VkDevice vk_device) {
            _vk_device = vk_device;
            return static_cast<Builder&>(*this);
        }

        Builder& add_vk_physical_device(VkPhysicalDevice vk_physical_device) {
            _vk_physical_device = vk_physical_device;
            return static_cast<Builder&>(*this);
        }

        Builder& add_vk_surface(VkSurfaceKHR vk_surface) {
            _vk_surface = vk_surface;
            return static_cast<Builder&>(*this);
        }

        Builder& add_vk_render_pass(VkRenderPass vk_render_pass) {
            _vk_render_pass = vk_render_pass;
            return static_cast<Builder&>(*this);
        }

    };

}