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

void Graphic::Vulkan_Assets_Manager::destroy_data(VkDevice vk_device) {
    Assets_Manager::destroy_data();
    _vk_texs_stroage->destroy_resources(vk_device);
}

Graphic::Vulkan_Texture* Graphic::Vulkan_Assets_Manager::load_vk_texture(
    std::string path,
    VkDevice vk_device,
    VkPhysicalDevice vk_physical_device,
    Vulkan_Queues* vk_queues,
    Vulkan_Command_Pool* vk_command_pool
) {
    Utility::Log::get()->log_info("load_vk_texture 1", vk_command_pool, vk_queues);
    Vulkan_Texture* tmp_tex = _vk_texs_stroage->get_template_resource(path);
    _texs_storage->load_resource(
        Core::Resource_Load_Mode::ASYNC,
        path,
        {path},
        [this, vk_physical_device, vk_device, vk_command_pool, vk_queues, path] (Texture *tex) {
            Utility::Log::get()->log_info("Vulkan_Assets_Manager::Vulkan_Assets_Manager 1", vk_physical_device, vk_device, vk_queues, vk_command_pool);
            _vk_texs_stroage->load_resource(
                Core::Resource_Load_Mode::ASYNC,
                path,
                {tex, vk_physical_device, vk_device, vk_queues, vk_command_pool},
                [path] (Vulkan_Texture* tex) {
                    Utility::Log::get()->log_info("Load texture finish", path, tex);
                }
            );
        }
    );
    return tmp_tex;
}