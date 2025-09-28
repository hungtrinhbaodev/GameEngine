#include <graphic/vulkan_implementation/vulkan_assets_manager.h>

Graphic::Vulkan_Assets_Manager::Vulkan_Assets_Manager() {
    _vk_texs_stroage = new Vulkan_Texture_Storage();
}

Graphic::Vulkan_Assets_Manager::~Vulkan_Assets_Manager() {
    delete(_vk_texs_stroage);
}


void Graphic::Vulkan_Assets_Manager::init_data() {
    Assets_Manager::init_data();
}

void Graphic::Vulkan_Assets_Manager::destroy_data() {
    Assets_Manager::destroy_data();
}

Graphic::Vulkan_Texture* Graphic::Vulkan_Assets_Manager::load_vk_texture(
    std::string path,
    VkDevice vk_device,
    VkPhysicalDevice vk_physical_device,
    Vulkan_Queues* vk_queues,
    Vulkan_Command_Pool* vk_command_pool
) {
    Utility::Log::get()->log_info("load_vk_texture 1", vk_command_pool, vk_queues);
    Vulkan_Texture *ret_tex = new Vulkan_Texture();
    _texs_storage->load_resource(
        Core::Resource_Load_Mode::ASYNC,
        path,
        {path},
        [&, vk_physical_device, vk_device, vk_command_pool, vk_queues] (Texture *tex) {
            Utility::Log::get()->log_info("Vulkan_Assets_Manager::Vulkan_Assets_Manager 1", vk_physical_device, vk_device, vk_queues, vk_command_pool);
            _vk_texs_stroage->load_resource(
                Core::Resource_Load_Mode::ASYNC,
                path,
                {tex, vk_physical_device, vk_device, vk_queues, vk_command_pool},
                [&] (Vulkan_Texture* vk_tex) {
                    delete(ret_tex);
                    ret_tex = vk_tex;
                }
            );
        }
    );
    return ret_tex;
}