#include <vulkan/vk_core.h>
#include <vulkan/vk_texture.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Vulkan {

    struct Texture_Load_Info {
        stbi_uc* pixels = nullptr;
        int width;
        int height;
        int channels;
    };

    Texture::Texture() {

    }

    Texture::Texture(const Texture& other) {
        name = other.name;
        inner_image = Image(other.inner_image);
    }

    void Texture::load_from(std::string file, std::string with_name) {

        name = with_name;

        auto texture_info = _global_thread_pool->enqueue([] (std::string file) {
            Texture_Load_Info texture_info{};
            texture_info.pixels = stbi_load(
                file.data(),
                &texture_info.width,
                &texture_info.height,
                &texture_info.channels,
                STBI_rgb_alpha
            );
            if (!texture_info.pixels) {
                throw std::runtime_error("Vulkan fail to load texture from file!");
            }
            return texture_info;
        }, file).get();

        inner_image.make_image(
            texture_info.width,
            texture_info.height,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT
        );

        inner_image.transition_image_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            inner_image.copy_image_data(texture_info.width, texture_info.height, texture_info.pixels);

        inner_image.transition_image_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    void Texture::destroy() {

        inner_image.destroy();
        
    }

}