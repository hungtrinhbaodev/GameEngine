#include <iostream>
#include <chrono>
#include <graphic/common/texture_system.h>
#include <graphic/vulkan_implementation/vulkan_core_data.h>

const std::string DEFAULT_PATH = "/Users/lap13994/Documents/hungtrinhbaodev/GameEngine/res/texture/";
int main() {
    auto vk_data = Graphic::Vulkan_Core_Data::get();
    vk_data->init_data();
    return 0;
}