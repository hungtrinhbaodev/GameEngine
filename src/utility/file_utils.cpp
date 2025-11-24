#include <utility/file_utils.h>

#ifdef __APPLE__
std::string Utility::File_Utils::DEFAULT_PATH = std::string("/Users/lap13994/Documents/hungtrinhbaodev/GameEngine/");
#else
std::string Utility::File_Utils::DEFAULT_PATH = std::string("D:/game_engine/game_engine/");
#endif // __APPLE__

Utility::File_Utils* Utility::File_Utils::_instance = nullptr;

Utility::File_Utils* Utility::File_Utils::get(){
    if (_instance == nullptr) {
        _instance = new File_Utils();
    }
    return _instance;
}

std::vector<char> Utility::File_Utils::read_file_shader(const std::string& path) {
    std::ifstream file{DEFAULT_PATH + path, std::ios::ate | std::ios::binary};
    
    if(!file.is_open()){
        throw std::runtime_error("Fail to open file: " + path);
    }

    size_t file_size = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);

    file.close();

    Utility::Log::get()->log_info("Load file success: ", path);
    return buffer;
}

void Utility::File_Utils::clean_up() {
    if (_instance) {
        delete(_instance);
    }
}