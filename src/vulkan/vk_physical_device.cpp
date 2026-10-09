#include <log.h>
#include <vulkan/vk_consts.h>
#include <vulkan/vk_physical_device.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

	void init_physical_device(VkPhysicalDevice& physical_device, VkInstance instance, VkSurfaceKHR surface) {

		uint32_t device_count = 0;
		vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

		if (device_count <= 0) {
			throw std::runtime_error("Vulkan fail to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> physical_devices(device_count);
		vkEnumeratePhysicalDevices(instance, &device_count, physical_devices.data());

		std::vector<std::string> physical_device_infos;
		for (VkPhysicalDevice physical_device : physical_devices) {
			VkPhysicalDeviceProperties property{};
			vkGetPhysicalDeviceProperties(physical_device, &property);
			physical_device_infos.push_back(property.deviceName);
		}
		Log::info("Vulkan list physical device:", physical_device_infos);

		for (VkPhysicalDevice current_physical_device : physical_devices) {
			if (Utils::is_suitable_physical_device(current_physical_device, surface)) {
				physical_device = current_physical_device;
				break;
			}
		}

		if (physical_device == VK_NULL_HANDLE) {
			Utils::vk_check_result(VK_INCOMPLETE, "", "Vulkan fail to find a suitable GPU!");
		} else {
			VkPhysicalDeviceProperties property;
			vkGetPhysicalDeviceProperties(physical_device, &property);
			Log::info("Vulkan chosed deviced: ", physical_device, property.deviceName);
		}
	}

} // namespace Vulkan