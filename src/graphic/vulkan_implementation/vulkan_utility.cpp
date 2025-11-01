#include <graphic/vulkan_implementation/vulkan_utility.h>
#include <graphic/vulkan_implementation/vulkan_core_data.h>


void Graphic::Vulkan_Utility::vk_check_action(VkResult result, std::string crash_message) {
    if (result != VK_SUCCESS) {
        Utility::Log::get()->log_info("Fail action result", result);
        throw std::runtime_error(crash_message.data());
    }
}

std::vector<const char*> Graphic::Vulkan_Utility::query_instance_extensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions_name_requires(glfwExtensions, glfwExtensions + glfwExtensionCount);

    uint32_t number_layer_extension = 0;
    vkEnumerateInstanceExtensionProperties(
        nullptr,
        &number_layer_extension,
        nullptr
    );

    std::vector<VkExtensionProperties> extension_properties(number_layer_extension);
    vkEnumerateInstanceExtensionProperties(
        nullptr,
        &number_layer_extension,
        extension_properties.data()
    );

    // check extension name require is in enabled extension
    std::vector<const char *> extension_names;
    for(auto extension_require : extensions_name_requires){
        for (auto extension_property : extension_properties){
            if(strcmp(extension_require, extension_property.extensionName) == 0){
                extension_names.push_back(extension_require);
            }
        }
    }

    // check specific flatform extension and push it back
    for (auto extension_property : extension_properties){
        if(strcmp(extension_property.extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0){
            extension_names.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        }
    }

    if(Vulkan_Constants::IS_ENABLED_VALIDATION_LAYER){
        extension_names.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extension_names;
}

std::vector<VkLayerProperties> Graphic::Vulkan_Utility::query_instance_layer_propeties() {
    uint32_t number_layer_properties;
    vkEnumerateInstanceLayerProperties(&number_layer_properties, nullptr);

    std::vector<VkLayerProperties> layer_properties(number_layer_properties);
    vkEnumerateInstanceLayerProperties(&number_layer_properties, layer_properties.data());

    return layer_properties;
}

std::vector<const char*> Graphic::Vulkan_Utility::query_instance_layer_enabled() {
    std::vector<const char *> list_layer_enabled;
    if(Vulkan_Constants::IS_ENABLED_VALIDATION_LAYER){
        list_layer_enabled.push_back(Vulkan_Constants::VALIDATION_LAYER_NAME);
    }
    return list_layer_enabled;
}

bool Graphic::Vulkan_Utility::is_validation_layer_enabled() {
    auto layer_properties = query_instance_layer_propeties();

    for (auto& layer_propertie : layer_properties) {
        if (strcmp(Vulkan_Constants::VALIDATION_LAYER_NAME, layer_propertie.layerName) == 0) {
            return true;
        }
    }

    return false;
}

std::vector<const char *> Graphic::Vulkan_Utility::query_physical_device_support_required_extensions(
    VkPhysicalDevice vk_physical_device
) {
    uint32_t number_extension = 0;
    vkEnumerateDeviceExtensionProperties(vk_physical_device, nullptr, &number_extension, nullptr);
    std::vector<VkExtensionProperties> extension_properties(number_extension);

    vkEnumerateDeviceExtensionProperties(vk_physical_device, nullptr, &number_extension, extension_properties.data());
    std::vector<const char *> extension_names;

    for(auto &extension_property : extension_properties){
        for(auto &extension_require : Vulkan_Constants::REQUIRED_PHYSICAL_DEVICE_EXTENSIONS){
            if(strcmp(extension_require, extension_property.extensionName) == 0){
                extension_names.push_back(extension_require);
            }
        }
    }

    return extension_names;
}

Graphic::Vulkan_Queue_Family_Indices Graphic::Vulkan_Utility::query_suitable_queue_family_indices(VkPhysicalDevice vk_physical_device, VkSurfaceKHR vk_surface) {
    uint32_t number_queue_family = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device, &number_queue_family, nullptr);

    std::vector<VkQueueFamilyProperties> family_queues(number_queue_family);
    vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device, &number_queue_family, family_queues.data());

    Vulkan_Queue_Family_Indices indices;
    for(int i = 0;i < family_queues.size();i++){
        VkQueueFamilyProperties *property = &family_queues[i];
        if (property->queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphic_family = i;
        }

        VkBool32 present_support = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(vk_physical_device, i, vk_surface, &present_support);
        if(present_support){
            indices.present_family = i;
        }

        if(indices.is_complete()){
            break;
        }
    }
    return indices;
}

Graphic::Vulkan_Swapchain_Support_Detail Graphic::Vulkan_Utility::query_swapchain_support_detail(
    VkPhysicalDevice vk_physical_device, 
    VkSurfaceKHR vk_surface
) {
    Vulkan_Swapchain_Support_Detail details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device, vk_surface, &details.capabilities);

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device, vk_surface, &format_count, nullptr);
    if(format_count > 0){
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device, vk_surface, &format_count, details.formats.data());
    }

    uint32_t present_modes_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device, vk_surface, &present_modes_count, nullptr);
    if(present_modes_count > 0){
        details.present_modes.resize(present_modes_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device, vk_surface, &present_modes_count, details.present_modes.data());
    }

    return details;
}

bool Graphic::Vulkan_Utility::is_suitable_physical_device(VkPhysicalDevice vk_physical_device, VkSurfaceKHR vk_surface) {
    // find queue suitable in device
    Vulkan_Queue_Family_Indices indices = query_suitable_queue_family_indices(vk_physical_device, vk_surface);

    // check device is pass require all list require extension is defined before
    const auto& list_extensions = query_physical_device_support_required_extensions(vk_physical_device);
    bool is_device_support_require_extension = list_extensions.size() >= Vulkan_Constants::REQUIRED_PHYSICAL_DEVICE_EXTENSIONS.size();

    // query capabilities of swap chain info in device
    bool swap_chain_adequate = false;
    if (is_device_support_require_extension) {
        Vulkan_Swapchain_Support_Detail details = query_swapchain_support_detail(
            vk_physical_device,
            vk_surface
        );
        swap_chain_adequate = !details.formats.empty() && !details.present_modes.empty();
    }

    VkPhysicalDeviceFeatures features{};
    vkGetPhysicalDeviceFeatures(vk_physical_device, &features);

    return swap_chain_adequate && is_device_support_require_extension && indices.is_complete() && features.samplerAnisotropy; 
}

std::vector<const char*> Graphic::Vulkan_Utility::query_physical_device_layers_enabled(VkPhysicalDevice vk_physical_device) {
    uint32_t number_layer = 0;
    vkEnumerateDeviceLayerProperties(vk_physical_device, &number_layer, nullptr);

    std::vector<VkLayerProperties> layer_enables(number_layer);
    vkEnumerateDeviceLayerProperties(vk_physical_device, &number_layer, layer_enables.data());

    std::vector<const char*> layer_names;
    for(auto &layer_enable : layer_enables){
        if(Vulkan_Constants::IS_ENABLED_VALIDATION_LAYER && strcmp(layer_enable.layerName, Vulkan_Constants::VALIDATION_LAYER_NAME)){
            layer_names.push_back(Vulkan_Constants::VALIDATION_LAYER_NAME);
        }
    }

    return layer_names;
}

VkImageView Graphic::Vulkan_Utility::create_imageview_from_image(VkImage vk_image, const VkFormat& vk_format, VkDevice vk_device) {

    // if device is NULL HANDLE try to get default device
    if (vk_device == VK_NULL_HANDLE) {
        const auto& vk_data = Vulkan_Core_Data::get()->get_raw_data();
        vk_device = vk_data.device;
    }

    // if device still null check init core data to create device
    if (vk_device == VK_NULL_HANDLE) {
        Vulkan_Utility::vk_check_action(
            VK_INCOMPLETE,
            "Can't find device to create image view from image!"
        );
    }

    VkImageViewCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = vk_image;
    create_info.format = vk_format;
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;
    create_info.subresourceRange.levelCount = 1;

    VkImageView image_view;
    Vulkan_Utility::vk_check_action(
        vkCreateImageView(vk_device, &create_info, nullptr, &image_view),
        "Fail to create image view!"
    );

    return image_view;
}

uint32_t Graphic::Vulkan_Utility::find_buffer_memory_type_index(
    uint32_t type_filter,
    VkMemoryAllocateFlags properties,
    VkPhysicalDevice vk_physical_device
) {

    const auto& vk_default_data = get_or_default_device(VK_NULL_HANDLE, vk_physical_device);
    vk_physical_device = vk_default_data.vk_physical_device;

    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(vk_physical_device, &memory_properties);

    for(int i = 0;i < memory_properties.memoryTypeCount;i++){
        if((type_filter & (i << 1)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties){
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

Graphic::Vulkan_Device_Default_Data Graphic::Vulkan_Utility::get_or_default_device(VkDevice vk_device, VkPhysicalDevice vk_physical_device) {
    if (vk_physical_device == VK_NULL_HANDLE || vk_device == VK_NULL_HANDLE) {
        const auto& vk_data = Vulkan_Core_Data::get()->get_raw_data();
        vk_physical_device = vk_data.physical_device;
        vk_device = vk_data.device;
    }

    if (vk_physical_device == VK_NULL_HANDLE || vk_device == VK_NULL_HANDLE) {
        Vulkan_Utility::vk_check_action(
            VK_INCOMPLETE,
            "Can't find default device or physical device"
        );
    }

    return {
        vk_device,
        vk_physical_device
    };
}

Graphic::Vulkan_Submit_Default_Data Graphic::Vulkan_Utility::get_or_default_submit(Vulkan_Queues* wp_queues, Vulkan_Command_Pool* wp_command_pool) {
    if (wp_queues == nullptr || wp_command_pool == nullptr) {
        const auto& wp_data = Vulkan_Core_Data::get()->get_wrapper_data();
        wp_queues = wp_data.wp_queues;
        wp_command_pool = wp_data.wp_command_pool;
    }

    if (wp_queues == nullptr || wp_command_pool == nullptr) {
        Vulkan_Utility::vk_check_action(
            VK_INCOMPLETE,
            "Can't find default wp queues or wp command pool"
        );
    }

    return {
        wp_queues,
        wp_command_pool
    };
}

Graphic::Vulkan_Commands_Mode Graphic::Vulkan_Utility::get_command_mode_by_load_resource_mode(Core::Resource_Load_Mode resource_mode) {
    switch(resource_mode) {
        case Core::Resource_Load_Mode::SYNC: {
            return Vulkan_Commands_Mode::COMMANDS_MODE_SYNC;
        }
        default: {
            return Vulkan_Commands_Mode::COMMANDS_MODE_ASYNC;
        }
    }
}