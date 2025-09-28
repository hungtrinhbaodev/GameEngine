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
    Graphic::Vulkan_Texture* vk_tex = vk_assets_mgr->load_vk_texture(
        DEFAULT_PATH + "texture1.png"
    );
    Graphic::Vulkan_Texture* vk_tex2 = vk_assets_mgr->load_vk_texture(
        DEFAULT_PATH + "texture1.png"
    );
    Graphic::Vulkan_Texture* vk_tex3 = vk_assets_mgr->load_vk_texture(
        DEFAULT_PATH + "texture2.png"
    );

    static bool is_log_unload = false, is_log_loaded = false;

    // main loop of game engine
    while(Graphic::Graphic::is_running()) {

        // pool user events
        glfwPollEvents();

        if (vk_tex->get_loaded_state() != Core::Resource_Loaded_State::LOADED && !is_log_unload) {
            Utility::Log::get()->log_info("Vulkan Texture is loading", vk_tex->get_loaded_state());
            is_log_unload = true;
        }

        if (vk_tex->get_loaded_state() == Core::Resource_Loaded_State::LOADED && !is_log_loaded) {
            Utility::Log::get()->log_info("Vulkan Texture is loaded", vk_tex->get_loaded_state());
            is_log_loaded = true;
        }
    }

    // clear data and clean up window
    window->clear_window();
    Graphic::Window::clean_up();

    // log to finish app
    Utility::Log::get()->log_info("App is closed successfully!");

    return 0;
}