#include <graphic/common/texture_system.h>

Graphic::Texture_System* Graphic::Texture_System::instance = nullptr;

Graphic::Texture_System::Texture_System() {

}

void Graphic::Texture_System::add_callback(std::string path, Callback callback) {
    if (callback == nullptr) return;

    callback_lock.lock();

    if (texture_callback.find(path) == texture_callback.end()) {
        texture_callback[path] = {};
    }

    for (auto& added_callback : texture_callback[path]) {
        if (&callback == &added_callback) {
            return;
        }
    }

    texture_callback[path].push_back(callback);

    callback_lock.unlock();
}

void Graphic::Texture_System::load_texture_threads(Texture_System* texture_sys, Texture* loading_texture, std::string path) {
    int width, height, channels;
    stbi_uc *pixels = stbi_load(
        path.data(),
        &width,
        &height,
        &channels,
        STBI_rgb_alpha
    );
    loading_texture->update_info_after_loaded(
        pixels,
        width,
        height,
        channels
    );
    loading_texture->set_loaded_state(Texture_Loaded_State::LOADED);
    texture_sys->do_callback(path, loading_texture);
}

void Graphic::Texture_System::do_callback(std::string path, Texture* texture) {
    callback_lock.lock();
    
    for (auto& callback : texture_callback[path]) {
        std::thread t(callback, texture);
        t.detach();
    }

    texture_callback.erase(path);

    callback_lock.unlock();
}

void Graphic::Texture_System::load_texture(std::string path, Callback callback) {

    storage_lock.lock();

    if (texture_storage.find(path) == texture_storage.end()) {
        Texture* texture = new Texture();
        texture_storage[path] = texture;
    }

    Texture *texture = texture_storage[path];
    switch (texture->get_loaded_state()) {
        case Texture_Loaded_State::UNLOADED: {
            add_callback(path, callback);
            texture->set_loaded_state(Texture_Loaded_State::LOADING);
            std::thread t(load_texture_threads, this, texture, path);
            t.detach();
            break;
        }
        case Texture_Loaded_State::LOADING: {
            add_callback(path, callback);
            break;
        }
        default: {
            std::thread t(callback, texture_storage[path]);
            t.detach();
        }
    }

    storage_lock.unlock();
}

Graphic::Texture_System* Graphic::Texture_System::get() {
    static std::mutex instance_lock;
    instance_lock.lock();

    if (instance == nullptr) {
        instance = new Texture_System();
    }
    
    instance_lock.unlock();
    return instance;
}

Graphic::Texture_System::~Texture_System() {
    storage_lock.lock();

    for (auto& item : texture_storage) {
        delete(item.second);
    }

    storage_lock.unlock();
}