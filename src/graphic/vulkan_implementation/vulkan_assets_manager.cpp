#include <graphic/vulkan_implementation/vulkan_assets_manager.h>

Graphic::Vulkan_Assets_Manager::Vulkan_Assets_Manager() {
    _vk_texs_storage = new Core::Resource_Storage<std::string, Vulkan_Texture, Vulkan_Texture_Load_Description>();
    _vk_mesh_storage = new Core::Resource_Storage<std::string, Vulkan_Mesh, Vulkan_Mesh_Load_Params>();
    _vk_model_storage = new Core::Resource_Storage<std::string, Vulkan_Model, Vulkan_Model_Load_Params>();
}

Graphic::Vulkan_Assets_Manager::~Vulkan_Assets_Manager() {
    delete(_vk_texs_storage);
    delete(_vk_mesh_storage);
    delete(_vk_model_storage);
}


void Graphic::Vulkan_Assets_Manager::init_data(Vulkan_Descriptor* wp_descriptor) {
    _wp_descriptor = wp_descriptor;
    Assets_Manager::init_data();
}

void Graphic::Vulkan_Assets_Manager::destroy_data(VkDevice vk_device) {
    _vk_texs_storage->destroy_resources(vk_device);
    _vk_mesh_storage->destroy_resources();
    _vk_model_storage->destroy_resources();
    Assets_Manager::destroy_data();
}

std::shared_ptr<Graphic::Vulkan_Texture> Graphic::Vulkan_Assets_Manager::load_vk_texture(
    Core::Resource_Load_Mode load_mode,
    std::string path
) {
    std::shared_ptr<Vulkan_Texture> tmp_tex = _vk_texs_storage->get_template_resource(path);
    _texs_storage->load_resource(
        load_mode,
        path,
        Texture_Load_Description {
            path
        },
        [this, path, load_mode, tmp_tex] 
        (std::shared_ptr<Texture> tex) {
            _vk_texs_storage->load_resource(
                load_mode,
                path,
                Vulkan_Texture_Load_Description {
                    load_mode,
                    tex,
                    [this, tmp_tex] () {
                        _wp_descriptor->update_texture_descriptor(tmp_tex);
                    }
                }
            );
        }
    );
    return tmp_tex;
}

std::shared_ptr<Graphic::Vulkan_Mesh> Graphic::Vulkan_Assets_Manager::load_vk_mesh(
    Core::Resource_Load_Mode load_mode,
    std::string path,
    Mesh_Buffer* vk_vertices_buffer,
    Mesh_Buffer* vk_indices_buffer
) {
    std::shared_ptr<Vulkan_Mesh> vk_mesh = _vk_mesh_storage->get_template_resource(path);
    _meshs_storage->load_resource(
        load_mode,
        path,
        Mesh_Load_Params {
            path
        },
        [this, load_mode, vk_vertices_buffer, vk_indices_buffer] (std::shared_ptr<Mesh> mesh) {
            _vk_mesh_storage->load_resource(
                load_mode,
                mesh->get_key(),
                Vulkan_Mesh_Load_Params {
                    mesh,
                    vk_vertices_buffer,
                    vk_indices_buffer
                }
            );
        }
    );
    return vk_mesh;
}

std::shared_ptr<Graphic::Vulkan_Model> Graphic::Vulkan_Assets_Manager::load_vk_model(
    Core::Resource_Load_Mode load_mode,
    std::string path_mesh,
    std::string path_texture,
    Mesh_Buffer* vk_vertices_buffer,
    Mesh_Buffer* vk_indices_buffer
) {

    Vulkan_Draw_ID draw_ID = Vulkan_Model::make_draw_id(path_mesh, path_texture);

    // load vulkan mesh asset
    std::shared_ptr<Vulkan_Mesh> vk_mesh = load_vk_mesh(
        load_mode,
        path_mesh,
        vk_vertices_buffer,
        vk_indices_buffer
    );

    // load vulkan texture asset
    std::shared_ptr<Vulkan_Texture> vk_texture = nullptr;
    switch (draw_ID) {
        case Vulkan_Draw_ID::OBJECT_WITH_TEXTURE: {
            vk_texture = load_vk_texture(
                load_mode,
                path_texture
            );
            break;
        }
        default: {
            break;
        }
    }

    // load vulkan model by dependencies
    return _vk_model_storage->load_resource(
        load_mode,
        Vulkan_Model::make_key(path_mesh, path_texture),
        Vulkan_Model_Load_Params {
            path_mesh,
            path_texture,
            vk_texture,
            vk_mesh
        }
    );
}