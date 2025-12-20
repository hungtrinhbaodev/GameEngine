#pragma once

#include <string>

#include <vulkan/vulkan.h>

#include <vulkan/vk_image.h>

namespace Vulkan {

    struct Texture {

        std::string name;

        Image inner_image;

        Texture();

        Texture(const Texture& other);

        void load_from(std::string file, std::string with_name);

        void destroy();

    };

}