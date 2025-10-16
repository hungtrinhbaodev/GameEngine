#include <graphic/common/assets_manager.h>

Graphic::Assets_Manager::Assets_Manager() {
    _texs_storage = new Core::Resource_Storage<std::string, Texture, Texture_Load_Description>();

    _meshs_storage = new Core::Resource_Storage<std::string, Mesh, Mesh_Load_Params>();
};

void Graphic::Assets_Manager::init_data() {

}

std::shared_ptr<Graphic::Texture> Graphic::Assets_Manager::load_texture(std::string path, Texture_Loaded_Callback callback) {
    return _texs_storage->load_resource(
        Core::Resource_Load_Mode::ASYNC,
        path,
        Texture_Load_Description{path},
        callback
    );
}

std::shared_ptr<Graphic::Mesh> Graphic::Assets_Manager::load_mesh(std::string path, Mesh_Loaded_Callback callback) {
    // Hard code here to load mesh to test
    double rand = Utility::Math_Utils::rand();
    if (rand >= 0.5f) {
        path = "TRIANGLE";
    }
    else {
        path = "RECTANGLE";
    }
    return _meshs_storage->load_resource(
        Core::Resource_Load_Mode::ASYNC,
        path,
        Mesh_Load_Params {path},
        callback
    );
}

void Graphic::Assets_Manager::destroy_data() {
    _texs_storage->destroy_resources();
    _meshs_storage->destroy_resources();
}

Graphic::Assets_Manager::~Assets_Manager() {
    delete(_texs_storage);
    delete(_meshs_storage);
};