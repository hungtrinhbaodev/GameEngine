#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <chrono>
#include <thread>

#include <graphic/common/texture.h>
#include <graphic/common/window.h>
#include <graphic/common/graphic.h>

const std::string DEFAULT_PATH = "/Users/lap13994/Documents/hungtrinhbaodev/GameEngine/res/texture/";

int main() {

    // init window singleton
    Graphic::Window* window = Graphic::Window::get();
    window->init_window("GameEngine", 1200, 720);
    
    // init graphic with window
    Graphic::Graphic::init(window);

    // enter main thread of graphic
    Graphic::Graphic::main(window);

    // test graphic
    const auto& vk_data = Graphic::Vulkan_Core_Data::get();
    const auto& vk_assets_mgr = vk_data->get_wrapper_data().wp_assets_mgr;
    Core::Resource_Load_Mode mode = Core::Resource_Load_Mode::SYNC;
    Core::Resource_Load_Mode mode2 = Core::Resource_Load_Mode::ASYNC;
    for (int i = 0;i < 1000; i++) {
        Graphic::Vulkan_Texture* vk_tex = vk_assets_mgr->load_vk_texture(
            mode2,
            DEFAULT_PATH + "texture1.png"
        );
        Graphic::Vulkan_Texture* vk_tex2 = vk_assets_mgr->load_vk_texture(
            mode,
            DEFAULT_PATH + "texture2.png"
        );
        Graphic::Vulkan_Texture* vk_tex3 = vk_assets_mgr->load_vk_texture(
            (i % 2) ? mode2 : mode,
            DEFAULT_PATH + "texture3.png"
        );
        Graphic::Vulkan_Texture* vk_tex4 = vk_assets_mgr->load_vk_texture(
            mode,
            DEFAULT_PATH + "texture4.png"
        );
        Graphic::Vulkan_Texture* vk_tex5 = vk_assets_mgr->load_vk_texture(
            mode,
            DEFAULT_PATH + "texture1.png"
        );
    }

    // main loop of game engine
    while(Graphic::Graphic::is_running()) {

        // pool user events
        glfwPollEvents();
    }

    // clear data and clean up window
    window->clear_window();
    Graphic::Window::clean_up();

    // log to finish app
    Utility::Log::get()->log_info("App is closed successfully!");

    return 0;
}