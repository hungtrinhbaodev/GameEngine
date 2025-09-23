#pragma once
#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>

#include <iostream>
#include <mutex>

#include <graphic/vulkan_implementation/vulkan_constants.h>
#include <graphic/vulkan_implementation/vulkan_utility.h>
#include <graphic/vulkan_implementation/vulkan_instance.h>
#include <graphic/vulkan_implementation/vulkan_physical_device.h>
#include <graphic/vulkan_implementation/vulkan_surface_khr.h>
#include <graphic/vulkan_implementation/vulkan_device.h>
#include <graphic/vulkan_implementation/vulkan_queues.h>
#include <graphic/vulkan_implementation/vulkan_swapchain.h>
#include <graphic/vulkan_implementation/vulkan_render_pass.h>
#include <graphic/vulkan_implementation/vulkan_frame_buffers.h>
#include <graphic/common/window.h>

namespace Graphic {

    using Vk_Utils = Vulkan_Utility;
     
    using Vk_Const = Vulkan_Constants;
    
    /**
     * All vulkan elements is wrapper,
     * wp is alias of wrapper
     */
    struct Vulkan_Wrapper_Data {
        
        Vulkan_Instance* wp_instance = nullptr;

        Vulkan_Surface_KHR* wp_surface = nullptr;

        Vulkan_Physical_Device* wp_physical_device = nullptr;

        Vulkan_Device* wp_device = nullptr;

        Vulkan_Queues* wp_queues = nullptr;

        Vulkan_Swapchain* wp_swapchain = nullptr;

        Vulkan_Render_Pass* wp_render_pass = nullptr;

        Vulkan_Frame_Buffers* wp_frame_buffers = nullptr;
    };

    /**
     * All vulkan element with raw handle
     */
    struct Vulkan_Raw_Data {
        
        VkInstance instance = VK_NULL_HANDLE;

        VkSurfaceKHR surface = VK_NULL_HANDLE;

        VkPhysicalDevice physical_device = VK_NULL_HANDLE;

        VkDevice device = VK_NULL_HANDLE;

        VkQueue graphics_queue = VK_NULL_HANDLE;

        VkQueue present_queue = VK_NULL_HANDLE;

        VkSwapchainKHR swapchain = VK_NULL_HANDLE;

        VkRenderPass render_pass = VK_NULL_HANDLE;

        const std::vector<VkFramebuffer>& frame_buffers;
    };

    class Vulkan_Core_Data {

        private:

        static Vulkan_Core_Data* _instance;

        static std::mutex _lock_instance;

        Window* _window;
        
        Vulkan_Instance* _vk_instance;

        Vulkan_Surface_KHR* _vk_surface;

        Vulkan_Physical_Device* _vk_physical_device;

        Vulkan_Device* _vk_device;

        Vulkan_Queues* _vk_queues;

        Vulkan_Swapchain* _vk_swapchain;

        Vulkan_Render_Pass* _vk_render_pass;

        Vulkan_Frame_Buffers* _vk_frame_buffers;

        public:

        void init_data(Window *window);

        void clear_data();

        Vulkan_Core_Data();

        ~Vulkan_Core_Data();

        Vulkan_Instance* get_instance();

        Vulkan_Physical_Device* get_physical_device();

        static Vulkan_Core_Data* get();

        Vulkan_Raw_Data get_raw_data();

        Vulkan_Wrapper_Data get_wrapper_data();

        static void clean_up();
    };

}