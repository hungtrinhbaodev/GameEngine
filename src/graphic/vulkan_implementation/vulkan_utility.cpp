#include <graphic/vulkan_implementation/vulkan_utility.h>

void Graphic::Vulkan_Utility::vk_check_action(VkResult result, std::string crash_message) {
    if (result != VK_SUCCESS) {
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