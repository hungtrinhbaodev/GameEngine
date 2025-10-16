#include <graphic/vulkan_implementation/vulkan_texture.h>

/**
 * Vulkan_Texture field
 */

void Graphic::Vulkan_Texture::on_finish_load() {

}

void Graphic::Vulkan_Texture::_make_vk_sampler(VkDevice vk_device, VkPhysicalDevice vk_physical_device) {
    const auto& vk_default_device = Vulkan_Utility::get_or_default_device(vk_device, vk_physical_device);
    vk_device = vk_default_device.vk_device;
    vk_physical_device = vk_default_device.vk_physical_device;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(vk_physical_device, &properties);

    VkSamplerCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    create_info.magFilter = VK_FILTER_LINEAR;
    create_info.minFilter = VK_FILTER_LINEAR;
    create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    create_info.anisotropyEnable = VK_FALSE;
    create_info.maxAnisotropy = 1.0f;
    create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    create_info.unnormalizedCoordinates = VK_FALSE;
    create_info.compareEnable = VK_FALSE;
    create_info.compareOp = VK_COMPARE_OP_ALWAYS;
    create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if(vkCreateSampler(vk_device, &create_info, nullptr, &_vk_sampler) != VK_SUCCESS){
        throw std::runtime_error("Fail to create sampler!");
    }
}

void Graphic::Vulkan_Texture::on_load(const Vulkan_Texture_Load_Description& des) {
    const auto& texture_info = des.texture->get_texture_info();
    Utility::Log::get()->log_info("load_vk_texture 1");

    auto vk_device_default = Vulkan_Utility::get_or_default_device(
        VK_NULL_HANDLE,
        VK_NULL_HANDLE
    );

    auto wp_sumit_default = Vulkan_Utility::get_or_default_submit(
        nullptr,
        nullptr
    );

    _vk_image.make(
        texture_info.width,
        texture_info.height,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    std::vector<Vulkan_Commands_Record_Data> records;

    records.emplace_back(
        _vk_image.make_transition_record_data(
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        )
    );

    records.emplace_back(
        _vk_image.make_copy_to_image_record_data(
            texture_info.width,
            texture_info.height,
            texture_info.pixels
        )
    );

    records.emplace_back(
        _vk_image.make_transition_record_data(
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            [this, vk_device_default, des] () {
                _make_vk_sampler(vk_device_default.vk_device, vk_device_default.vk_physical_device);
                set_loaded_state(Core::Resource_Loaded_State::LOADED);
                Utility::Log::get()->log_info("Loaded Vulkan Texture finish: ", des.texture->get_path());
            }
        )
    );

    wp_sumit_default.command_pool->record_sequence_commands(
        Vulkan_Utility::get_command_mode_by_load_resource_mode(des.load_mode),
        records
    );
}

void Graphic::Vulkan_Texture::destroy(VkDevice vk_device) {
    
    _vk_image.destroy(vk_device);

    if (_vk_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(vk_device, _vk_sampler, nullptr);
    }
}