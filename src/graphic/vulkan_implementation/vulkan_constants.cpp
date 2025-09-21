#include <graphic/vulkan_implementation/vulkan_constants.h>

bool Graphic::Vulkan_Constants::IS_ENABLED_VALIDATION_LAYER = true;

const char* Graphic::Vulkan_Constants::VALIDATION_LAYER_NAME = "VK_LAYER_KHRONOS_validation";

std::vector<const char*> Graphic::Vulkan_Constants::REQUIRED_PHYSICAL_DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
};

int Graphic::Vulkan_Constants::MAX_FRAMES_IN_FLIGHT = 2;