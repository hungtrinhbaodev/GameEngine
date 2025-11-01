#include <graphic/vulkan_implementation/vulkan_swapchain.h>

VkSurfaceFormatKHR Graphic::Vulkan_Swapchain::choose_swapchain_format(const std::vector<VkSurfaceFormatKHR>& available_formats) {
    for (const VkSurfaceFormatKHR& available_format : available_formats) {
        if (available_format.format == VK_FORMAT_B8G8R8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return available_format;
        }
    }
    return available_formats[0];
}

VkPresentModeKHR Graphic::Vulkan_Swapchain::choose_swapchain_present_mode(const std::vector<VkPresentModeKHR>& available_presents) {
    for (const VkPresentModeKHR& available_present : available_presents) {
        if (available_present == VK_PRESENT_MODE_MAILBOX_KHR) {
            return VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Graphic::Vulkan_Swapchain::choose_swapchain_extent(const VkSurfaceCapabilitiesKHR& capabilites, GLFWwindow* window) {
    if (capabilites.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilites.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    VkExtent2D actual_extent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };
    actual_extent.width = std::clamp(actual_extent.width, capabilites.currentExtent.width, capabilites.maxImageExtent.width);
    actual_extent.height = std::clamp(actual_extent.height, capabilites.currentExtent.height, capabilites.maxImageExtent.height);
    return actual_extent;
}

void Graphic::Vulkan_Swapchain::_create_swapchain_image_views(
    const std::vector<VkImage>& swapchain_images, 
    const VkFormat& format,
    VkDevice vk_device
) {
    _vk_swapchain_imageviews.resize(swapchain_images.size());

    for(size_t i = 0;i < swapchain_images.size();i++){
        _vk_swapchain_imageviews[i] = Vulkan_Utility::create_imageview_from_image(
            swapchain_images[i], 
            format,
            vk_device
        );
    }
}

void Graphic::Vulkan_Swapchain::init(
    VkPhysicalDevice vk_physical_device,
    VkSurfaceKHR vk_surface,
    VkDevice vk_device,
    GLFWwindow *window
) {
    Vulkan_Swapchain_Support_Detail swapchain_detail = Vulkan_Utility::query_swapchain_support_detail(
        vk_physical_device,
        vk_surface
    );
    Utility::Log::get()->log_info("Swapchain detail info:");
    swapchain_detail.log_info();

    VkSurfaceFormatKHR format = choose_swapchain_format(swapchain_detail.formats);
    VkPresentModeKHR present = choose_swapchain_present_mode(swapchain_detail.present_modes);
    VkExtent2D extent = choose_swapchain_extent(swapchain_detail.capabilities, window);

    // log mode present is choosen
    switch (present)
    {
        case VK_PRESENT_MODE_MAILBOX_KHR: {
            Utility::Log::get()->log_info("Swapchain choose mode present: VK_PRESENT_MODE_MAILBOX_KHR");
            break;
        }
        default: {
            Utility::Log::get()->log_info("Swapchain choose mode present: VK_PRESENT_MODE_FIFO_KHR");
            break;
        }
    }

    uint32_t image_count = swapchain_detail.capabilities.minImageCount + 1;
    if(swapchain_detail.capabilities.maxImageCount > 0 && image_count > swapchain_detail.capabilities.maxImageCount){
        image_count = swapchain_detail.capabilities.maxImageCount;
    }

    Utility::Log::get()->log_info("Vulkan_Swapchain::init", swapchain_detail.capabilities.maxImageCount, swapchain_detail.capabilities.minImageCount);

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = vk_surface;

    create_info.minImageCount = image_count;
    create_info.imageFormat = format.format;
    create_info.imageColorSpace = format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    Vulkan_Queue_Family_Indices indices = Vulkan_Utility::query_suitable_queue_family_indices(
        vk_physical_device,
        vk_surface
    );
    uint32_t queue_indices[] = {indices.graphic_family.value(), indices.present_family.value()};
    if (indices.graphic_family != indices.present_family) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_indices;
        Utility::Log::get()->log_info("Swapchain choose image sharing mode: VK_SHARING_MODE_CONCURRENT");
    }
    else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        Utility::Log::get()->log_info("Swapchain choose image sharing mode: VK_SHARING_MODE_EXCLUSIVE");
    }

    create_info.preTransform = swapchain_detail.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present;
    create_info.clipped = VK_TRUE;

    create_info.oldSwapchain = VK_NULL_HANDLE;

    Vulkan_Utility::vk_check_action(
        vkCreateSwapchainKHR(vk_device, &create_info, nullptr, &_vk_swapchain),
        "failed to create swap chain!"
    );
    Utility::Log::get()->log_info("Create swap chain successfully!");

    vkGetSwapchainImagesKHR(vk_device, _vk_swapchain, &image_count, nullptr);
    _vk_swapchain_images.resize(image_count);
    vkGetSwapchainImagesKHR(vk_device, _vk_swapchain, &image_count, _vk_swapchain_images.data());
    Utility::Log::get()->log_info("swapchain image count", image_count);
    
    _vk_swapchain_format = format.format;
    _vk_swapchain_extent = extent;

    _create_swapchain_image_views(
        _vk_swapchain_images, 
        _vk_swapchain_format, 
        vk_device
    );
}

void Graphic::Vulkan_Swapchain::recreate_swapchain(
    VkPhysicalDevice vk_physical_device,
    VkSurfaceKHR vk_surface,
    VkDevice vk_device,
    GLFWwindow* window
) {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(vk_device);
    destroy(vk_device);
    init(
        vk_physical_device,
        vk_surface,
        vk_device,
        window
    );
}

VkSwapchainKHR Graphic::Vulkan_Swapchain::get() {
    return _vk_swapchain;
}

VkFormat Graphic::Vulkan_Swapchain::get_format() {
    return _vk_swapchain_format;
}

VkExtent2D Graphic::Vulkan_Swapchain::get_extent() {
    return _vk_swapchain_extent;
}

std::vector<VkImageView>& Graphic::Vulkan_Swapchain::get_imageviews() {
    return _vk_swapchain_imageviews;
}

void Graphic::Vulkan_Swapchain::destroy(VkDevice vk_device) {

    for(auto& image_view : _vk_swapchain_imageviews){
        vkDestroyImageView(vk_device, image_view, nullptr);
    }
    Utility::Log::get()->log_info("Destroy swap chain image view success!");

    vkDestroySwapchainKHR(vk_device, _vk_swapchain, nullptr);
    Utility::Log::get()->log_info("Destroy swap chain success!");
}