#include <set>

#include <vulkan/vk_core.h>
#include <vulkan/vk_device.h>
#include <vulkan/vk_consts.h>
#include <vulkan/vk_utils.h>
#include <log.h>

namespace Vulkan {

	namespace Init {

		void _init_device() {

			Queue_Family_Indices indices = Utils::query_suitable_queue_family_indices(physical_device, surface);

			std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
			std::set<uint32_t> unique_queue_family = {indices.graphic_family.value(), indices.present_family.value()};
			float queue_priority = 1.0f;
			for (uint32_t queue_family_index : unique_queue_family) {
				VkDeviceQueueCreateInfo queue_create_info{};
				queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queue_create_info.queueFamilyIndex = queue_family_index;
				queue_create_info.queueCount = 1;
				queue_create_info.pQueuePriorities = &queue_priority;
				queue_create_infos.push_back(queue_create_info);
			}

			VkPhysicalDeviceFeatures device_features{};

			VkDeviceCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			create_info.queueCreateInfoCount = queue_create_infos.size();
			create_info.pQueueCreateInfos = queue_create_infos.data();
			create_info.pEnabledFeatures = &device_features;

			std::vector<const char*> device_extensions =
				Utils::query_physical_device_support_required_extensions(physical_device);
			create_info.enabledExtensionCount = device_extensions.size();
			create_info.ppEnabledExtensionNames = device_extensions.data();

#if !defined(_WIN32)
			std::vector<const char*> device_layers_enabled =
				Utils::query_physical_device_layers_enabled(physical_device);
			create_info.enabledLayerCount = device_layers_enabled.size();
			create_info.ppEnabledLayerNames = device_layers_enabled.data();
#endif

			Utils::vk_check_result(
				vkCreateDevice(physical_device, &create_info, nullptr, &device),
				"Vulkan create logical device success!", "Vulkan fail to create logical device!"
			);
		}

	} // namespace Init

	namespace Destroy {

		void _destroy_device() {

			vkDestroyDevice(device, nullptr);
			Log::log_info("Vulkan destroy logical device success!");
		}

	} // namespace Destroy

} // namespace Vulkan
