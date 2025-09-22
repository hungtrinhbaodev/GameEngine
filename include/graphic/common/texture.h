#pragma once
#include <stb_image.h>

#include <string>
#include <iostream>

namespace Graphic {

    enum Texture_Loaded_State {
        UNLOADED = 0,
        LOADING = 1,
        LOADED = 2
    };

    class Texture {

        private:

        Texture_Loaded_State state = Texture_Loaded_State::UNLOADED;
        
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

        void load_texture(std::string path);

        void set_loaded_state(Texture_Loaded_State state);

        void update_info_after_loaded(stbi_uc* pixels, int width, int height, int channels);

        size_t get_texture_memory_size();

        Texture_Loaded_State get_loaded_state();

        ~Texture();
    };
};