#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <chrono>
#include <thread>

#include <utility/func_utils.h>
#include <graphic/common/texture.h>
#include <graphic/common/window.h>
#include <graphic/common/graphic.h>
#include <graphic/common/mesh.h>
#include <graphic/vulkan_implementation/vulkan_mesh_buffer.hpp>

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
    std::shared_ptr<Graphic::Mesh> mesh = vk_assets_mgr->load_mesh("", [] (std::shared_ptr<Graphic::Mesh> mesh) {
        Graphic::Vulkan_Mesh_Buffer<std::string> dynamic_buffer;
        Utility::Log::get()->log_info("Test dynamic buffer 0");
        dynamic_buffer.make(
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        Utility::Log::get()->log_info(
            "Vertices data:", 
            Utility::Func_Utils::parse_data<int>(mesh->get_indices_data(), 0,  mesh->get_indices_memory_size())
        );
        Utility::Log::get()->log_info(
            "Vertices data:", 
            Utility::Func_Utils::parse_data<int>(mesh->get_vertices_data(), 0,  mesh->get_vertices_memory_size())
        );
        dynamic_buffer.push_data("vertices", mesh->get_vertices_data(), 0, mesh->get_vertices_memory_size());
        dynamic_buffer.push_data("indices", mesh->get_indices_data(), 0, mesh->get_indices_memory_size());
        dynamic_buffer.delete_data("vertices");
        dynamic_buffer.push_data("indices 2", mesh->get_indices_data(), 0, mesh->get_indices_memory_size());
        dynamic_buffer.delete_data("indices");
        dynamic_buffer.push_data("vertices", mesh->get_vertices_data(), 0, mesh->get_vertices_memory_size());
        dynamic_buffer.destroy();
    });


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