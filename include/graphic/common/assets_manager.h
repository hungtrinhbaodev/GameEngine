#pragma once
#include <mutex>

#include <graphic/common/texture.h>

namespace Graphic {

    using Texture_Loaded_Callback = std::function<void(Texture*)>;
    
    class Assets_Manager {

        protected:

        Texture_Storage* _texs_storage;

        public:

        Assets_Manager();

        void init_data();

        Texture* load_texture(
            std::string path_texture,
            Texture_Loaded_Callback callback = nullptr
        );

        void destroy_data();

        ~Assets_Manager();
    };
    
}