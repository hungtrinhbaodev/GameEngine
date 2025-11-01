#include <graphic/vulkan_implementation/vulkan_model.h>



std::string Graphic::Vulkan_Model::make_key(std::string path_mesh, std::string path_texture) {
    return path_mesh + path_texture;
}

Graphic::Vulkan_Draw_ID Graphic::Vulkan_Model::make_draw_id(std::string path_mesh, std::string path_texture) {
    if (path_texture == "") {
        return Vulkan_Draw_ID::OBJECT_DEFAULT;
    }
    return Vulkan_Draw_ID::OBJECT_WITH_TEXTURE;
}

void Graphic::Vulkan_Model::on_load(const Vulkan_Model_Load_Params& load_param) {
    _vk_texture = load_param.tex;
    _vk_mesh = load_param.mesh;
    _draw_id = make_draw_id(load_param.path_mesh, load_param.path_texture);
}

Graphic::Vulkan_Draw_ID Graphic::Vulkan_Model::get_draw_id() {
    return _draw_id;
}

Core::Resource_Loaded_State Graphic::Vulkan_Model::get_loaded_state() {
    switch(_loaded_state) {
        case Core::Resource_Loaded_State::LOADED: {
            // if state of model is loaded
            // we need to check state of
            // it's dependencies
            switch (get_draw_id()) {
                case Vulkan_Draw_ID::OBJECT_DEFAULT: {
                    if (
                        _vk_mesh != nullptr
                        && _vk_mesh->get_loaded_state() == Core::Resource_Loaded_State::LOADED
                    ) {
                        return Core::Resource_Loaded_State::LOADED;
                    }
                    break;
                }
                default: {
                    if (
                        _vk_texture != nullptr 
                        && _vk_mesh != nullptr
                        && _vk_texture->get_loaded_state() == Core::Resource_Loaded_State::LOADED
                        && _vk_mesh->get_loaded_state() == Core::Resource_Loaded_State::LOADED
                    ) {
                        return Core::Resource_Loaded_State::LOADED;
                    }
                }
            }
            return Core::Resource_Loaded_State::LOADING;
        }
        default: {
            return Core::Resource<Vulkan_Model_Load_Params, std::string>::get_loaded_state();
        }
    }
}

std::shared_ptr<Graphic::Vulkan_Texture> Graphic::Vulkan_Model::get_vk_texture() {
    return _vk_texture;
}

std::shared_ptr<Graphic::Vulkan_Mesh> Graphic::Vulkan_Model::get_vk_mesh() {
    return _vk_mesh;
}