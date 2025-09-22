#pragma once
#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>

#include <iostream>
#include <mutex>

#include <graphic/vulkan_implementation/vulkan_constants.h>
#include <graphic/vulkan_implementation/vulkan_utility.h>
#include <graphic/vulkan_implementation/vulkan_instance.h>
#include <graphic/common/window.h>

namespace Graphic {

    using Vk_Utils = Vulkan_Utility;
     
    using Vk_Const = Vulkan_Constants;

    class Vulkan_Core_Data {

        private:

        Window* window;

        static Vulkan_Core_Data* _instance;
        
        Vulkan_Instance* _vk_instance;

        public:

        void init_data(Window *window);

        void clear_data();

        Vulkan_Core_Data();

        ~Vulkan_Core_Data();

        Vulkan_Instance* get_instance();

        static Vulkan_Core_Data* get();
    };

}