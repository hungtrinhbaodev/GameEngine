#include <graphic/common/assets_manager.h>

Graphic::Assets_Manager::Assets_Manager() {
    _texs_storage = new Texture_Storage();

};

void Graphic::Assets_Manager::init_data() {

}

Graphic::Texture* Graphic::Assets_Manager::load_texture(std::string path, Texture_Loaded_Callback callback) {
    return _texs_storage->load_resource(
        Core::Resource_Load_Mode::ASYNC,
        path,
        {path},
        callback
    );
}

void Graphic::Assets_Manager::destroy_data() {
    
}


Graphic::Assets_Manager::~Assets_Manager() {
    delete(_texs_storage);
};