#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <chrono>
#include <thread>

#include <graphic/common/texture_system.h>
#include <graphic/vulkan_implementation/vulkan_core_data.h>
#include <utility/log_utils.h>
#include <utility/glm_utils.h>
#include <graphic/common/window.h>

const std::string DEFAULT_PATH = "/Users/lap13994/Documents/hungtrinhbaodev/GameEngine/res/texture/";
int main() {
    // init window singleton
    auto window = Graphic::Window::get();
    window->init_window("GameEngine", 1200, 720);

    // init vulkan data singleton
    auto data = Graphic::Vulkan_Core_Data::get();
    data->init_data(window);

    // main loop of game engine
    while(window->is_running()) {
        glfwPollEvents();
    }
    
    // clear data singleton 
    data->clear_data();
    window->clear_window();

    // clean up delete instance
    Graphic::Vulkan_Core_Data::clean_up();
    Graphic::Window::clean_up();

    return 0;
}