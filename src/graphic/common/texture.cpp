#include <graphic/common/texture.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <graphic/common/texture_system.h>

Graphic::Texture::Texture() {

}

Graphic::Texture::Texture(const std::string &path_tex) {
    this->path_tex = path_tex;
}

Graphic::Texture::Texture(const char *path_tex) {
    this->path_tex = std::string(path_tex);
}

Graphic::Texture::Texture(const Texture &tex) {
    state = tex.state;
    path_tex = tex.path_tex;
    width = tex.width;
    height = tex.height;
    pixels = tex.pixels;
}

std::string& Graphic::Texture::get_path() {
    return path_tex;
}

void Graphic::Texture::set_loaded_state(Texture_Loaded_State state) {
    this->state = state;
}

Graphic::Texture_Loaded_State Graphic::Texture::get_loaded_state() {
    return state;
}

void Graphic::Texture::load_texture(std::string path) {
    path_tex = path;
    state = Texture_Loaded_State::LOADING;
    Texture_System::get()->load_texture(path, [this](Texture* tex) {
        update_info_after_loaded(
            tex->pixels,
            tex->width,
            tex->height,
            tex->channels
        );
        set_loaded_state(Texture_Loaded_State::LOADED);
        std::cout << "Load texture success: " << path_tex << " " << get_texture_memory_size() << std::endl;
    });
}

void Graphic::Texture::update_info_after_loaded(stbi_uc* pixels, int width, int height, int channels) {
    this->pixels = pixels;
    this->channels = channels;
    this->width = width;
    this->height = height;
}

size_t Graphic::Texture::get_texture_memory_size() {
    return static_cast<size_t>(width) * height * channels;
}

Graphic::Texture::~Texture() {
    
}


