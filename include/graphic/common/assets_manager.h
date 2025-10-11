#pragma once
#include <mutex>

#include <graphic/common/texture.h>

namespace Graphic {

    using Texture_Loaded_Callback = std::function<void(std::shared_ptr<Texture>)>;
    
    class Assets_Manager {

        protected:

        Core::Resource_Storage<std::string, Texture, Texture_Load_Description>* _texs_storage;

        public:

        Assets_Manager();

        void init_data();

        std::shared_ptr<Texture> load_texture(
            std::string path_texture,
            Texture_Loaded_Callback callback = nullptr
        );

        void destroy_data();

        ~Assets_Manager();
    };
    
}