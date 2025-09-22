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
    Utility::Log::get()->log_info("Hi Iam Hung", 1, 5.0, 0.5, glm::mat4(1.0f), glm::vec3(2.0f));
    // std::cout << glm::mat4(1.0f) << std::endl;
    return 0;
}