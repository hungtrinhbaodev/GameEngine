#include <vulkan/vk_core.h>
#include <vulkan/vk_instance.h>
#include <vulkan/vk_consts.h>
#include <vulkan/vk_utils.h>

#include <log.h>

namespace Vulkan {

	namespace Init {

		void _init_instance() {

			// check layer debug is supported
			if (Const::IS_ENABLE_VALIDATION_LAYERS && !Utils::is_validation_layer_enabled()) {
				Utils::vk_check_result(VK_INCOMPLETE, "", "Vulkan validation layers requested, but not available!");
				return;
			}

			// create application info
			VkApplicationInfo app_info{};
			app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			app_info.pApplicationName = "Game Engine";
			app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
			app_info.pEngineName = "No Engine";
			app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);

#if !defined(_WIN32)
			app_info.apiVersion = VK_API_VERSION_1_1;
#else
			app_info.apiVersion = VK_API_VERSION_1_0;
#endif

			VkInstanceCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			create_info.pApplicationInfo = &app_info;

			// add extesion instance info
			std::vector<const char*> extensions = Utils::query_instance_extensions();

			// modify flag extension by specific flatform require extension
#if !defined(_WIN32)
			for (auto extension : extensions) {
				if (strcmp(extension, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0) {
					create_info.flags |= VK_KHR_portability_enumeration;
				}
			}
#endif
			Log::log_info("Vulkan list instance extension:", extensions);
			create_info.enabledExtensionCount = extensions.size();
			create_info.ppEnabledExtensionNames = extensions.data();

			// add layer instance info
			std::vector<const char*> layerEnableds = Utils::query_instance_layer_enabled();
			Log::log_info("Vulkan list instance layer:", layerEnableds);
			create_info.enabledLayerCount = layerEnableds.size();
			create_info.ppEnabledLayerNames = layerEnableds.data();

			// add debug utils for instance
			if (Const::IS_ENABLE_VALIDATION_LAYERS) {
				VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
				_populate_debug_messenger_create_info(debug_create_info);
				create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
			}

			// create vulkan instance with create info
			Utils::vk_check_result(vkCreateInstance(&create_info, nullptr, &instance),
								   "Vulkan create instance success!", "Vulkan fail to create instance!");

			if (Const::IS_ENABLE_VALIDATION_LAYERS) {
				_set_up_vulkan_debuger_messenger(instance, nullptr, &debug_messenger);
			}
		}

		VkResult _create_debug_messeger_ext(VkInstance instance,
											const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
											const VkAllocationCallbacks* p_allocator,
											VkDebugUtilsMessengerEXT* p_debug_messenger) {
			auto func =
				(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
			if (func != nullptr) {
				return func(instance, p_create_info, p_allocator, p_debug_messenger);
			} else {
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		}

		VKAPI_ATTR VkBool32 VKAPI_CALL _debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
													   VkDebugUtilsMessageTypeFlagsEXT message_type,
													   const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
													   void* p_user_data) {
			std::cerr << "Vulkan validation layer: " << p_callback_data->pMessage << std::endl;
			return VK_FALSE;
		}

		void _populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info) {
			create_info = {};
			create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
										  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
										  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
									  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
									  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			create_info.pfnUserCallback = _debug_callback;
		}

		void _set_up_vulkan_debuger_messenger(VkInstance instance, const VkAllocationCallbacks* p_allocator,
											  VkDebugUtilsMessengerEXT* p_debug_messenger) {
			VkDebugUtilsMessengerCreateInfoEXT create_info{};
			_populate_debug_messenger_create_info(create_info);

			Utils::vk_check_result(_create_debug_messeger_ext(instance, &create_info, p_allocator, p_debug_messenger),
								   "Vulkan set up messenger set up successfully!",
								   "Vulkan fail to set up debug messenger!");
		}
	} // namespace Init

	namespace Destroy {

		void _destroy_debug_messeger_ext(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger,
										 const VkAllocationCallbacks* p_allocator) {
			auto func =
				(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
			if (func != nullptr) {
				func(instance, debug_messenger, p_allocator);
			}
		}

		void _destroy_instance() {
			if (Const::IS_ENABLE_VALIDATION_LAYERS) {
				_destroy_debug_messeger_ext(instance, debug_messenger, nullptr);
				Log::log_info("Vulkan debug messenger destroyed successfully!");
			}
			vkDestroyInstance(instance, nullptr);
			Log::log_info("Vulkan instance destroyed successfully!");
		}
	} // namespace Destroy
} // namespace Vulkan