#include <graphic/common/texture_system.h>

Graphic::TextureSystem::TextureSystem() {

}

void Graphic::TextureSystem::add_callback(std::string path, Callback callback) {
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

void Graphic::TextureSystem::load_texture_threads(TextureSystem* texture_sys, Texture* loading_texture, std::string path) {
    loading_texture->load_texture(path);
    loading_texture->set_loaded_state(TextureLoadedState::LOADED);
    texture_sys->do_callback(path, loading_texture);
}

void Graphic::TextureSystem::do_callback(std::string path, Texture* texture) {
    callback_lock.lock();
    
    for (auto& callback : texture_callback[path]) {
        std::thread t(callback, texture);
        t.detach();
    }

    texture_callback.erase(path);

    callback_lock.unlock();
}

void Graphic::TextureSystem::load_texture(std::string path, Callback callback) {

    storage_lock.lock();

    if (texture_storage.find(path) == texture_storage.end()) {
        Texture* texture = new Texture();
        texture_storage[path] = texture;
    }

    Texture *texture = texture_storage[path];
    switch (texture->get_loaded_state()) {
        case TextureLoadedState::UNLOADED: {
            add_callback(path, callback);
            texture->set_loaded_state(TextureLoadedState::LOADING);
            std::thread t(load_texture_threads, this, texture, path);
            t.detach();
            break;
        }
        case TextureLoadedState::LOADING: {
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

Graphic::TextureSystem::~TextureSystem() {
    storage_lock.lock();
    
    for (auto& item : texture_storage) {
        delete(item.second);
    }

    storage_lock.unlock();
}