#pragma once
#include <stb_image.h>

#include <string>
#include <iostream>

#include <core/resource.hpp>
#include <core/resource_storage.hpp>
#include <utility/log_utils.h>

namespace Graphic {

    struct Texture_Load_Description {
        std::string tex_path = "";
    };

    struct Texture_View_Info {
        stbi_uc* pixels;
        int width;
        int height;
        int channels;
    };

    class Texture : public Core::Resource {

        using Texture_Loaded_Callback = std::function<void(Texture *)>;

        private:
        
        std::string _tex_path;

        int _width;
        
        int _height;

        int _channels;

        stbi_uc* _pixels = nullptr;

        public:

        Texture();

        void load_texture(std::string path);

        std::string& get_path();

        size_t get_texture_memory_size();

        Texture_View_Info get_texture_info();

        void destroy();

        ~Texture();

        friend class Texture_Loader;
    };

    class Texture_Storage : public Core::Resource_Storage<std::string, Texture, Texture_Load_Description, Texture_Storage> {

        public:

        void _load_resource (
            Texture* texture,
            const Texture_Load_Description& description
        );

    };
};