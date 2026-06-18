#pragma once

#include <vulkan/vulkan.h>

namespace Vulkan {

	namespace Init {

		void _init_instance();

		VkResult _create_debug_messeger_ext(VkInstance instance,
											const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
											const VkAllocationCallbacks* p_allocator,
											VkDebugUtilsMessengerEXT* p_debug_messenger);

		VKAPI_ATTR VkBool32 VKAPI_CALL _debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
													   VkDebugUtilsMessageTypeFlagsEXT message_type,
													   const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
													   void* p_user_data);

		void _populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info);

		void _set_up_vulkan_debuger_messenger(VkInstance instance, const VkAllocationCallbacks* p_allocator,
											  VkDebugUtilsMessengerEXT* p_debug_messenger);
	} // namespace Init

	namespace Destroy {

		void _destroy_debug_messeger_ext(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger,
										 const VkAllocationCallbacks* p_allocator);

		void _destroy_instance();
	} // namespace Destroy
} // namespace Vulkan
