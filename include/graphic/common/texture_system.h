#pragma once
#include <graphic/common/texture.h>
#include <thread>
#include <map>
#include <mutex>
#include <vector>
#include <functional>

namespace Graphic {

    using Callback = std::function<void(Texture*)>;

    class TextureSystem {

        private:

        std::mutex storage_lock;

        std::mutex callback_lock;
        
        std::map<std::string, Texture*> texture_storage;

        std::map<std::string, std::vector<Callback>> texture_callback;

        static void load_texture_threads(TextureSystem* texture_sys, Texture* loading_texture, std::string path);

        public:

        TextureSystem();

        void do_callback(std::string path, Texture* texture);

        void add_callback(std::string path, Callback callback = nullptr);

        void load_texture(std::string path, Callback callback = nullptr);

        ~TextureSystem();
    };

}