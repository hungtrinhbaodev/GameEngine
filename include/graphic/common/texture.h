#pragma once
#include <string>
#include <stb_image.h>
#include <iostream>

namespace Graphic {

    enum TextureLoadedState {
        UNLOADED = 0,
        LOADING = 1,
        LOADED = 2
    };

    class Texture {

        private:

        TextureLoadedState state = TextureLoadedState::UNLOADED;
        
        std::string path_tex;

        int width;
        
        int height;

        int channels;

        stbi_uc* pixels = nullptr;

        public:

        Texture();

        Texture(const std::string& path_tex);

        Texture(const char *path_tex);

        Texture(const Texture &tex);

        std::string& get_path();

        void load_texture(std::string &path);

        void set_loaded_state(TextureLoadedState state);

        size_t get_texture_memory_size();

        TextureLoadedState get_loaded_state();

        ~Texture();
    };
};