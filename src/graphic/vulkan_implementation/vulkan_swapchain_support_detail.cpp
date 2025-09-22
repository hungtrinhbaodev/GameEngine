#include <graphic/vulkan_implementation/vulkan_swapchain_support_detail.h>

void Graphic::Vulkan_Swapchain_Support_Detail::log_info() {
    Utility::Log::get()->log_info("Info swap chain of device: ", "\nformat size: ", formats.size(), "\npresent_modes_size: ", present_modes.size());
}