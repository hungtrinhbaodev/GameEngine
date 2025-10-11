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

    _vk_image.make(
        texture_info.width,
        texture_info.height,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    auto vk_device_default = Vulkan_Utility::get_or_default_device(
        VK_NULL_HANDLE,
        VK_NULL_HANDLE
    );

    auto wp_sumit_default = Vulkan_Utility::get_or_default_submit(
        nullptr,
        nullptr
    );
    Utility::Log::get()->log_info("load_vk_texture 1.2", wp_sumit_default.command_pool, wp_sumit_default.queues);

    _vk_image.transition_image_layout(
        Vulkan_Utility::get_command_mode_by_load_resource_mode(des.load_mode),
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        [this, des, texture_info, vk_device_default]() {

            Utility::Log::get()->log_info("load_vk_texture 2");

            VkDeviceSize image_size = texture_info.width * texture_info.height * 4;

            auto staging_buffer = std::make_shared<Vulkan_Buffer>();

            Utility::Log::get()->log_info("load_vk_texture 2.1");

            staging_buffer->make(
                image_size,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );

            staging_buffer->map_and_copy_data(
                texture_info.pixels,
                image_size
            );

            Utility::Log::get()->log_info("load_vk_texture 2.2");
            
            _vk_image.copy_buffer_to_image(
                Vulkan_Utility::get_command_mode_by_load_resource_mode(des.load_mode),
                texture_info.width,
                texture_info.height,
                staging_buffer,
                [this, des, staging_buffer, vk_device_default]() {

                    // destroy buffer staging when copy finish
                    staging_buffer->destroy();

                    Utility::Log::get()->log_info("load_vk_texture 3");
                    _vk_image.transition_image_layout(
                        Vulkan_Utility::get_command_mode_by_load_resource_mode(des.load_mode),
                        VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        [this, des, vk_device_default]() {
                            _make_vk_sampler(vk_device_default.vk_device, vk_device_default.vk_physical_device);
                            set_loaded_state(Core::Resource_Loaded_State::LOADED);
                            Utility::Log::get()->log_info("Loaded Vulkan Texture finish: ", des.texture->get_path());
                        }
                    );
                }
            );
        }
    );
}

void Graphic::Vulkan_Texture::destroy(VkDevice vk_device) {
    
    _vk_image.destroy(vk_device);

    if (_vk_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(vk_device, _vk_sampler, nullptr);
    }
}