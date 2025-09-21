#include <iostream>

#include <graphic/vulkan_implementation/vulkan_constants.h>
#include <graphic/vulkan_implementation/vulkan_utility.h>

namespace Graphic {

    using Vk_Utils = Vulkan_Utility;
     
    using Vk_Const = Vulkan_Constants;

    class Vulkan_Instance {
        private:

        VkInstance _vk_instance;

        static VkResult _create_debug_messeger_ext(
            VkInstance instance, 
            const VkDebugUtilsMessengerCreateInfoEXT* p_create_info, 
            const VkAllocationCallbacks* p_allocator, 
            VkDebugUtilsMessengerEXT* p_debug_messenger
        );

        static void _destroy_debug_messeger_ext(
            VkInstance instance, 
            VkDebugUtilsMessengerEXT debug_messenger, 
            const VkAllocationCallbacks* p_allocator
        );
        
        static VKAPI_ATTR VkBool32 VKAPI_CALL _debug_callback(
            VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, 
            VkDebugUtilsMessageTypeFlagsEXT message_type, 
            const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_user_data
        );

        static void _populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT &create_info);

        static void _set_up_vulkan_debuger_messenger(
            VkInstance instance,
            const VkAllocationCallbacks* p_allocator, 
            VkDebugUtilsMessengerEXT* p_debug_messenger
        );

        public:

        void init();

        void destroy();

        VkInstance get();
    };

};