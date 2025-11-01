#include <graphic/vulkan_implementation/vulkan_constants.h>

bool Graphic::Vulkan_Constants::IS_ENABLED_VALIDATION_LAYER = true;

const char* Graphic::Vulkan_Constants::VALIDATION_LAYER_NAME = "VK_LAYER_KHRONOS_validation";

std::vector<const char*> Graphic::Vulkan_Constants::REQUIRED_PHYSICAL_DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
};

int Graphic::Vulkan_Constants::MAX_FRAMES_IN_FLIGHT = 3;

std::vector<VkDynamicState> Graphic::Vulkan_Constants::PIPELINE_DYNAMIC_STATES = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
};

std::string Graphic::Vulkan_Constants::DEFAULT_PATH_SHADER_DRAW_DEFAULT = "res/shader/draw_default/";

std::string Graphic::Vulkan_Constants::DEFAULT_PATH_SHADER_DRAW_WITH_TEXTURE = "res/shader/draw_with_texture/";

uint32_t Graphic::Vulkan_Constants::MAX_NUMBER_DESCRIPTOR_TEXTURE = 1000;
