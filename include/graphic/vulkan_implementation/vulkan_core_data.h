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
#include <graphic/common/window.h>

namespace Graphic {

    using Vk_Utils = Vulkan_Utility;
     
    using Vk_Const = Vulkan_Constants;

    class Vulkan_Core_Data {

        private:

        static Vulkan_Core_Data* _instance;

        static std::mutex _lock_instance;

        Window* _window;
        
        Vulkan_Instance* _vk_instance;

        Vulkan_Surface_KHR* _vk_surface;

        Vulkan_Physical_Device* _vk_physical_device;

        public:

        void init_data(Window *window);

        void clear_data();

        Vulkan_Core_Data();

        ~Vulkan_Core_Data();

        Vulkan_Instance* get_instance();

        Vulkan_Physical_Device* get_physical_device();

        static Vulkan_Core_Data* get();

        static void clean_up();
    };

}