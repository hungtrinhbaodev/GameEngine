#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <chrono>
#include <thread>

#include <core/objects_id_generated.hpp>
#include <utility/func_utils.h>
#include <graphic/common/texture.h>
#include <graphic/common/window.h>
#include <graphic/common/graphic.h>
#include <graphic/common/mesh.h>
#include <graphic/vulkan_implementation/vulkan_mesh_buffer.hpp>
#include <graphic/vulkan_implementation/vulkan_instances_buffer.hpp>

const std::string DEFAULT_PATH = "/Users/lap13994/Documents/hungtrinhbaodev/GameEngine/res/texture/";

int main() {

    // init window singleton
    Graphic::Window* window = Graphic::Window::get();
    window->init_window("GameEngine", 1200, 720);
    
    // init graphic with window
    Graphic::Graphic::init(window);

    // enter main thread of graphic
    Graphic::Graphic::main(window);

    // main loop of game engine
    while(Graphic::Graphic::is_running()) {

        // pool user events
        glfwPollEvents();
    }

    // terminate to wait main thread graphic end
    Graphic::Graphic::terminate();

    // clear data and clean up window
    window->clear_window();
    Graphic::Window::clean_up();

    // log to finish app
    Utility::Log::get()->log_info("App is closed successfully!");

    return 0;
}