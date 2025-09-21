#include <iostream>
#include <chrono>
#include <graphic/common/texture_system.h>

const std::string DEFAULT_PATH = "/Users/lap13994/Documents/hungtrinhbaodev/GameEngine/res/texture/";
int main() {
    Graphic::Texture tex1, tex2, tex3;
    tex1.load_texture(DEFAULT_PATH + "texture1.png");
    tex2.load_texture(DEFAULT_PATH + "texture1.png");
    tex3.load_texture(DEFAULT_PATH + "texture2.png");
    static int count = 0;
    while(true) {
        if (count < 20) {
            std::cout << "Loaded state: " << tex1.get_loaded_state() << " " << tex2.get_loaded_state() << " " << tex3.get_loaded_state() << std::endl;
            count++;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return 0;
}