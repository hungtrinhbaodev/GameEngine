#pragma once

#include <iostream>
#include <vector>

#include <log.h>
#include <vulkan/queue_family_indices.h>
#include <vulkan/swapchain_support_detail.h>
#include <vulkan/vk_consts.h>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

namespace Vulkan {

	namespace Utils {

		inline void vk_check_result(
			VkResult result, const char* success_msg = "Vulkan action success!",
			const char* exception_msg = "Vulkan action fail!"
		) {
			switch (result) {
				case VK_SUCCESS: {
					if (strcmp("", success_msg) != 0) {
						std::cout << success_msg << std::endl;
					}
					break;
				}
				default: {
					throw std::runtime_error(exception_msg);
				}
			}
		}

		inline std::vector<VkLayerProperties> query_instance_layer_properties() {
			uint32_t number_layer_properties;
			vkEnumerateInstanceLayerProperties(&number_layer_properties, nullptr);

			std::vector<VkLayerProperties> layer_properties(number_layer_properties);
			vkEnumerateInstanceLayerProperties(&number_layer_properties, layer_properties.data());

			return layer_properties;
		}

		inline bool is_validation_layer_enabled() {
			auto layer_properties = query_instance_layer_properties();

			for (auto& layer_propertie : layer_properties) {
				if (strcmp(Const::VALIDATION_LAYER_NAME, layer_propertie.layerName) == 0) {
					return true;
				}
			}

			return false;
		}

		inline std::vector<const char*> query_instance_extensions() {
			uint32_t glfwExtensionCount = 0;
			const char** glfwExtensions;
			glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
			std::vector<const char*> extensions_name_requires(glfwExtensions, glfwExtensions + glfwExtensionCount);

			uint32_t number_layer_extension = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &number_layer_extension, nullptr);

			std::vector<VkExtensionProperties> extension_properties(number_layer_extension);
			vkEnumerateInstanceExtensionProperties(nullptr, &number_layer_extension, extension_properties.data());

			// check extension name require is in enabled extension
			std::vector<const char*> extension_names;
			for (auto extension_require : extensions_name_requires) {
				for (auto& extension_property : extension_properties) {
					if (strcmp(extension_require, extension_property.extensionName) == 0) {
						extension_names.push_back(extension_require);
					}
				}
			}

			// check specific flatform extension and push it back
			for (auto& extension_property : extension_properties) {
				if (strcmp(extension_property.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0) {
					extension_names.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
				}
			}

			if (Const::IS_ENABLE_VALIDATION_LAYERS) {
				extension_names.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}

			return extension_names;
		}

		inline std::vector<const char*> query_instance_layer_enabled() {
			std::vector<const char*> list_layer_enabled;
			if (Const::IS_ENABLE_VALIDATION_LAYERS) {
				list_layer_enabled.push_back(Const::VALIDATION_LAYER_NAME);
			}
			return list_layer_enabled;
		}

		inline Queue_Family_Indices query_suitable_queue_family_indices(
			VkPhysicalDevice physical_device, VkSurfaceKHR surface
		) {

			uint32_t number_queue_family = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &number_queue_family, nullptr);

			std::vector<VkQueueFamilyProperties> family_queues(number_queue_family);
			vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &number_queue_family, family_queues.data());

			Queue_Family_Indices indices;
			for (int i = 0; i < family_queues.size(); i++) {
				VkQueueFamilyProperties property = family_queues[i];
				if (property.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					indices.graphic_family = i;
				}

				VkBool32 present_support = VK_FALSE;
				vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
				if (present_support) {
					indices.present_family = i;
				}

				if (indices.is_complete()) {
					break;
				}
			}

			return indices;
		}

		inline std::vector<const char*> query_physical_device_support_required_extensions(
			VkPhysicalDevice physical_device
		) {
			uint32_t number_extension = 0;
			vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &number_extension, nullptr);
			std::vector<VkExtensionProperties> extension_properties(number_extension);

			vkEnumerateDeviceExtensionProperties(
				physical_device, nullptr, &number_extension, extension_properties.data()
			);
			std::vector<const char*> extension_names;

			for (auto& extension_property : extension_properties) {
				for (auto& extension_require : Const::REQUIRED_PHYSICAL_DEVICE_EXTENSIONS) {
					if (strcmp(extension_require, extension_property.extensionName) == 0) {
						extension_names.push_back(extension_require);
					}
				}
			}

			return extension_names;
		}

		inline Swapchain_Support_Detail query_swapchain_support_detail(
			VkPhysicalDevice physical_device, VkSurfaceKHR surface
		) {
			Swapchain_Support_Detail details;
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &details.capabilities);

			uint32_t format_count = 0;
			vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
			if (format_count > 0) {
				details.formats.resize(format_count);
				vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, details.formats.data());
			}

			uint32_t present_modes_count = 0;
			vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_modes_count, nullptr);
			if (present_modes_count > 0) {
				details.present_modes.resize(present_modes_count);
				vkGetPhysicalDeviceSurfacePresentModesKHR(
					physical_device, surface, &present_modes_count, details.present_modes.data()
				);
			}

			return details;
		}

		inline bool is_suitable_physical_device(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
			// find queue suitable in device
			Queue_Family_Indices indices = query_suitable_queue_family_indices(physical_device, surface);

			// check device is pass require all list require extension is defined before
			const auto& list_extensions = query_physical_device_support_required_extensions(physical_device);
			bool is_device_support_require_extension =
				list_extensions.size() >= Const::REQUIRED_PHYSICAL_DEVICE_EXTENSIONS.size();

			// query capabilities of swap chain info in device
			bool swap_chain_adequate = false;
			if (is_device_support_require_extension) {
				Swapchain_Support_Detail details = query_swapchain_support_detail(physical_device, surface);
				swap_chain_adequate = !details.formats.empty() && !details.present_modes.empty();
			}

			VkPhysicalDeviceFeatures features{};
			vkGetPhysicalDeviceFeatures(physical_device, &features);

			return swap_chain_adequate && is_device_support_require_extension && indices.is_complete() &&
				   features.samplerAnisotropy;
		}

		inline std::vector<const char*> query_physical_device_layers_enabled(VkPhysicalDevice physical_device) {
			uint32_t number_layer = 0;
			vkEnumerateDeviceLayerProperties(physical_device, &number_layer, nullptr);

			std::vector<VkLayerProperties> layer_enables(number_layer);
			vkEnumerateDeviceLayerProperties(physical_device, &number_layer, layer_enables.data());

			std::vector<const char*> layer_names;
			for (auto& layer_enable : layer_enables) {
				if (Const::IS_ENABLE_VALIDATION_LAYERS &&
					strcmp(layer_enable.layerName, Const::VALIDATION_LAYER_NAME)) {
					layer_names.push_back(Const::VALIDATION_LAYER_NAME);
				}
			}

			return layer_names;
		}

		inline VkImageView create_imageview_from_image(
			VkImage image, const VkFormat& format, VkImageAspectFlags aspect_flags, VkDevice device
		) {

			VkImageViewCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			create_info.image = image;
			create_info.format = format;
			create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			create_info.subresourceRange.aspectMask = aspect_flags;
			create_info.subresourceRange.baseMipLevel = 0;
			create_info.subresourceRange.baseArrayLayer = 0;
			create_info.subresourceRange.layerCount = 1;
			create_info.subresourceRange.levelCount = 1;

			VkImageView image_view;
			Utils::vk_check_result(
				vkCreateImageView(device, &create_info, nullptr, &image_view), "", "Fail to create image view!"
			);

			return image_view;
		}

		inline uint32_t find_suitable_memory_type(
			uint32_t type_filter, VkMemoryAllocateFlags properties, VkPhysicalDevice physical_device
		) {

			VkPhysicalDeviceMemoryProperties memory_properties;
			vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

			for (int i = 0; i < memory_properties.memoryTypeCount; i++) {
				Log::info(
					"find_suitable_memory_type", type_filter, memory_properties.memoryTypes[i].propertyFlags, properties
				);
				if ((type_filter & (1 << i)) &&
					(memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
					return i;
				}
			}

			throw std::runtime_error("Vulkan fail to find suitable memory type!");
		}

		inline VkFormat find_supported_format(
			const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features,
			VkPhysicalDevice physical_device
		) {
			for (auto format : candidates) {
				VkFormatProperties props;
				vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);
				if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
					return format;

				if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
					return format;
				}
			}

			throw std::runtime_error("Vulkan fail to find supported format!");
		}

		inline VkFormat find_depth_format(VkPhysicalDevice physical_deivce) {
			return find_supported_format(
				{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
				VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, physical_device
			);
		}

	} // namespace Utils

} // namespace Vulkan