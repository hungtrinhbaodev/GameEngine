#include <graphic/vulkan_implementation/vulkan_instance.h>

VkResult Graphic::Vulkan_Instance::_create_debug_messeger_ext(
    VkInstance instance, 
    const VkDebugUtilsMessengerCreateInfoEXT* p_create_info, 
    const VkAllocationCallbacks* p_allocator, 
    VkDebugUtilsMessengerEXT* p_debug_messenger
) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, p_create_info, p_allocator, p_debug_messenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void Graphic::Vulkan_Instance::_destroy_debug_messeger_ext(
    VkInstance instance, 
    VkDebugUtilsMessengerEXT debug_messenger, 
    const VkAllocationCallbacks* p_allocator
) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debug_messenger, p_allocator);
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL Graphic::Vulkan_Instance::_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, 
    VkDebugUtilsMessageTypeFlagsEXT message_type, 
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_user_data
) {
    std::cerr << "validation layer: " << p_callback_data->pMessage << std::endl;
    return VK_FALSE;
}

void Graphic::Vulkan_Instance::_populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT &create_info) {
    create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = _debug_callback;
}

void Graphic::Vulkan_Instance::_set_up_vulkan_debuger_messenger(
    VkInstance instance,
    const VkAllocationCallbacks* p_allocator, 
    VkDebugUtilsMessengerEXT* p_debug_messenger
) {
    VkDebugUtilsMessengerCreateInfoEXT create_info{};
    _populate_debug_messenger_create_info(create_info);

    Vulkan_Utility::vk_check_action(
        _create_debug_messeger_ext(
            instance,
            &create_info,
            p_allocator,
            p_debug_messenger
        ),
        "failed to set up debug messenger!"
    );
}

void Graphic::Vulkan_Instance::init() {

    // check layer debug is supported
    if (Vk_Const::IS_ENABLED_VALIDATION_LAYER && !Vk_Utils::is_validation_layer_enabled()) {
        Vk_Utils::vk_check_action(VK_INCOMPLETE, "validation layers requested, but not available!");
        return;
    }

    // create application info
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Game Engine";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    // add extesion instance info
    std::vector<const char*> extensions = Vk_Utils::query_instance_extensions();

    // modify flag extension by specific flatform require extension
    for (auto extension : extensions){
        if(strcmp(extension, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0){
            create_info.flags |= VK_KHR_portability_enumeration;
        }
    }
    create_info.enabledExtensionCount = extensions.size();
    create_info.ppEnabledExtensionNames = extensions.data();

    // add layer instance info
    std::vector<const char*> layerEnableds = Vulkan_Utility::query_instance_layer_enabled();
    create_info.enabledLayerCount = layerEnableds.size();
    create_info.ppEnabledLayerNames = layerEnableds.data();

    // add debug utils for instance
    if(Vk_Const::IS_ENABLED_VALIDATION_LAYER){
        VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
        _populate_debug_messenger_create_info(debug_create_info);
        create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debug_create_info;
    }

    // create vulkan instance with create info
    Vk_Utils::vk_check_action(
        vkCreateInstance(&create_info, nullptr, &_vk_instance),
        "failed to create instance!"
    );
}

void Graphic::Vulkan_Instance::destroy() {
    vkDestroyInstance(_vk_instance, nullptr);
}

VkInstance Graphic::Vulkan_Instance::get() {
    return _vk_instance;
}