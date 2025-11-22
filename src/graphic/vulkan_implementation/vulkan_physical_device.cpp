#include <graphic/vulkan_implementation/vulkan_physical_device.h>

void Graphic::Vulkan_Physical_Device::init(VkInstance vk_instance, VkSurfaceKHR vk_surface) {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(vk_instance, &device_count, nullptr);
    if(device_count <= 0){
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> physical_devices(device_count);
    vkEnumeratePhysicalDevices(vk_instance, &device_count, physical_devices.data());

    std::vector<std::string> physical_device_infos;
    for(VkPhysicalDevice physical_device : physical_devices){
        VkPhysicalDeviceProperties property{};
        vkGetPhysicalDeviceProperties(physical_device, &property);
        physical_device_infos.push_back(property.deviceName);
    }
    Utility::Log::get()->log_info("List physical device:", physical_device_infos);
    
    for(VkPhysicalDevice physical_device : physical_devices){
        if(Vulkan_Utility::is_suitable_physical_device(
            physical_device,
            vk_surface
        )){
            _vk_physical_device = physical_device;
            break;
        }
    }

    if (_vk_physical_device == VK_NULL_HANDLE) {
        Vulkan_Utility::vk_check_action(
            VK_INCOMPLETE,
            "failed to find a suitable GPU!"
        );
    }
    else{
        VkPhysicalDeviceProperties property;
        vkGetPhysicalDeviceProperties(_vk_physical_device, &property);
        Utility::Log::get()->log_info("Chosed deviced: ",  property.deviceName);
    }

 };

VkPhysicalDevice Graphic::Vulkan_Physical_Device::get() {
    return _vk_physical_device;
}