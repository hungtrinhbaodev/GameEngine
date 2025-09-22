#include <iostream>
#include <chrono>
#include <graphic/common/texture_system.h>
#include <graphic/vulkan_implementation/vulkan_core_data.h>
#include <utility/log_utils.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <thread>
#include <utility/glm_utils.h>

const std::string DEFAULT_PATH = "/Users/lap13994/Documents/hungtrinhbaodev/GameEngine/res/texture/";
int main() {
    auto data = Graphic::Vulkan_Core_Data::get();
    data->init_data();
    data->clear_data();
    return 0;
}