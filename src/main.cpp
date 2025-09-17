#include <iostream>
#include <graphic/common/texture_system.h>

const std::string DEFAUL_PATH = "/Users/lap13994/Documents/hungtrinhbaodev/GameEngine/res/texture/";
int main() {
    Graphic::TextureSystem* tex_sys = new Graphic::TextureSystem();
    tex_sys->load_texture(DEFAUL_PATH + "texture1.png", [&](Graphic::Texture *tex) {
        std::cout << "Loaded texture success 1: " << tex_sys << " " << tex->get_path() << " " << tex->get_texture_memory_size() << std::endl;
    });
    tex_sys->load_texture(DEFAUL_PATH + "texture1.png", [&](Graphic::Texture *tex) {
        std::cout << "Loaded texture success 2: " << tex_sys << " " <<  tex->get_path() << " " << tex->get_texture_memory_size() << std::endl;
    });
    tex_sys->load_texture(DEFAUL_PATH + "texture2.png", [&](Graphic::Texture *tex) {
        std::cout << "Loaded texture success 3: " << tex_sys << " " << tex->get_path() << " " << tex->get_texture_memory_size() << std::endl;
    });
    return 0;
}