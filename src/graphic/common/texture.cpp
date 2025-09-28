#include <graphic/common/texture.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

/**
 * Texture field
 */

Graphic::Texture::Texture() {

}

void Graphic::Texture::load_texture(std::string path) {

    Utility::Log::get()->log_info("texture path 1", _tex_path);

    _tex_path = path;

    Utility::Log::get()->log_info("texture path 2", _tex_path);

    _pixels = stbi_load(
        path.data(),
        &_width,
        &_height,
        &_channels,
        STBI_rgb_alpha
    );

    if (!_pixels) {
        throw std::runtime_error("Fail to load texture!");
    }
}

std::string& Graphic::Texture::get_path() {
    return _tex_path;
}

size_t Graphic::Texture::get_texture_memory_size() {
    return static_cast<size_t>(_width) * _height * _channels;
}

Graphic::Texture_View_Info Graphic::Texture::get_texture_info() {
    return {
        _pixels,
        _width,
        _height,
        _channels
    };
}

void Graphic::Texture::destroy() {
    delete(_pixels);
}

Graphic::Texture::~Texture() {
    
}

/**
 * Texture_Storage field
 */

void Graphic::Texture_Storage::_load_resource(
    Texture* texture, 
    const Texture_Load_Description& description
) {
    texture->load_texture(description.tex_path);
}