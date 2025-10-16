#include <graphic/vulkan_implementation/vulkan_assets_manager.h>

Graphic::Vulkan_Assets_Manager::Vulkan_Assets_Manager() {
    _vk_texs_stroage = new Core::Resource_Storage<std::string, Vulkan_Texture, Vulkan_Texture_Load_Description>();
}

Graphic::Vulkan_Assets_Manager::~Vulkan_Assets_Manager() {
    delete(_vk_texs_stroage);
}


void Graphic::Vulkan_Assets_Manager::init_data() {
    Assets_Manager::init_data();
}

void Graphic::Vulkan_Assets_Manager::destroy_data(VkDevice vk_device) {
    Assets_Manager::destroy_data();
    _vk_texs_stroage->destroy_resources(vk_device);
}

std::shared_ptr<Graphic::Vulkan_Texture> Graphic::Vulkan_Assets_Manager::load_vk_texture(
    Core::Resource_Load_Mode load_mode,
    std::string path
) {
    // Utility::Log::get()->log_info("load_vk_texture 1", vk_command_pool, vk_queues);
    std::shared_ptr<Vulkan_Texture> tmp_tex = _vk_texs_stroage->get_template_resource(path);
    _texs_storage->load_resource(
        load_mode,
        path,
        Texture_Load_Description {
            path
        },
        [this, path, load_mode] 
        (std::shared_ptr<Texture> tex) {
            // Utility::Log::get()->log_info("Vulkan_Assets_Manager::Vulkan_Assets_Manager 1", _vk_texs_stroage->get_number_resource());
            _vk_texs_stroage->load_resource(
                load_mode,
                path,
                Vulkan_Texture_Load_Description {
                    load_mode,
                    tex
                }
            );
        }
    );
    return tmp_tex;
}