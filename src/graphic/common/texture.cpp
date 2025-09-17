#include <graphic/common/texture.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

void Graphic::Texture::set_loaded_state(TextureLoadedState state) {
    this->state = state;
}

Graphic::TextureLoadedState Graphic::Texture::get_loaded_state() {
    return state;
}

void Graphic::Texture::load_texture(std::string &path) {
    path_tex = path;
    pixels = stbi_load(
        path.data(),
        &width,
        &height,
        &channels,
        STBI_rgb_alpha
    );
}

size_t Graphic::Texture::get_texture_memory_size() {
    return static_cast<size_t>(width) * height * channels;
}

Graphic::Texture::~Texture() {
    
}


