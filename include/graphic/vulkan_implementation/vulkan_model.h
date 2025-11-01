#pragma once
#include <core/resource.hpp>
#include <graphic/vulkan_implementation/vulkan_texture.h>
#include <graphic/vulkan_implementation/vulkan_mesh.h>

namespace Graphic {
    
    struct Vulkan_Model_Load_Params {

        std::string path_mesh = "";

        std::string path_texture = "";

        std::shared_ptr<Vulkan_Texture> tex;

        std::shared_ptr<Vulkan_Mesh> mesh;
    };

    class Vulkan_Model : public Core::Resource<Vulkan_Model_Load_Params, std::string> {

        private:

        std::shared_ptr<Vulkan_Texture> _vk_texture = nullptr;

        std::shared_ptr<Vulkan_Mesh> _vk_mesh = nullptr;

        Vulkan_Draw_ID _draw_id;

        public:

        static std::string make_key(std::string path_mesh, std::string path_texture);

        static Vulkan_Draw_ID make_draw_id(std::string path_mesh, std::string path_texture = "");

        Vulkan_Draw_ID get_draw_id();

        void on_load(const Vulkan_Model_Load_Params& params);

        Core::Resource_Loaded_State get_loaded_state();

        std::shared_ptr<Vulkan_Texture> get_vk_texture();

        std::shared_ptr<Vulkan_Mesh> get_vk_mesh();

    };
}