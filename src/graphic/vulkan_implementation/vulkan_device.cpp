#include <graphic/vulkan_implementation/vulkan_device.h>

void Graphic::Vulkan_Device::init(VkPhysicalDevice vk_physical_device, VkSurfaceKHR vk_surface) {
    Vulkan_Queue_Family_Indices indices = Vulkan_Utility::query_suitable_queue_family_indices(
        vk_physical_device,
        vk_surface
    );

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
    
    std::vector<const char *> device_extensions = Vulkan_Utility::query_physical_device_support_required_extensions(vk_physical_device);
    create_info.enabledExtensionCount = device_extensions.size();
    create_info.ppEnabledExtensionNames = device_extensions.data();

    std::vector<const char *> device_layers_enabled = Vulkan_Utility::query_physical_device_layers_enabled(vk_physical_device);
    create_info.enabledLayerCount = device_layers_enabled.size();
    create_info.ppEnabledLayerNames = device_layers_enabled.data();

    Vulkan_Utility::vk_check_action(
        vkCreateDevice(vk_physical_device, &create_info, nullptr, &_vk_device),
        "failed to create logical device!"
    );
    Utility::Log::get()->log_info("Create logical device success!");
}

VkDevice Graphic::Vulkan_Device::get() {
    return _vk_device;
}

void Graphic::Vulkan_Device::destroy() {
    vkDestroyDevice(_vk_device, nullptr);
    Utility::Log::get()->log_info("Destroy logical device success!");
}