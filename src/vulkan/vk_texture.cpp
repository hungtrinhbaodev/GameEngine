#include <vulkan/vk_core.h>
#include <vulkan/vk_texture.h>
#include <stb_image.h>
#include <log.h>


namespace Vulkan {

    struct Texture_Load_Info {

        stbi_uc* pixels = nullptr;
        int width;
        int height;
        int channels;

        void load(const std::string& file) {
            pixels = stbi_load(
                file.data(),
                &width,
                &height,
                &channels,
                STBI_rgb_alpha
            );
            if (!pixels) {
                throw std::runtime_error("Vulkan fail to load texture from file!");
            }
        }

        void release() {
            if (pixels != nullptr) {
                delete(pixels);
            }
            pixels = nullptr;
        }
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
            texture_info.load(file);
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

        /*
            Update sampler and update descriptor
            for image when it load successfully
        */
        {
            inner_image.make_sampler();
            inner_image.update_descriptor();
        }

        texture_info.release();
    }

    void Texture::destroy() {

        inner_image.destroy();
        
    }

}