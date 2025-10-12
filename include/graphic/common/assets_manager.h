#pragma once
#include <mutex>

#include <graphic/common/texture.h>
#include <graphic/common/mesh.h>
#include <utility/math_utils.h>

namespace Graphic {

    using Texture_Loaded_Callback = std::function<void(std::shared_ptr<Texture>)>;

    using Mesh_Loaded_Callback = std::function<void(std::shared_ptr<Mesh>)>;
    
    class Assets_Manager {

        protected:

        Core::Resource_Storage<std::string, Texture, Texture_Load_Description>* _texs_storage;

        Core::Resource_Storage<std::string, Mesh, Mesh_Load_Params>* _meshs_storage;

        public:

        Assets_Manager();

        void init_data();

        std::shared_ptr<Texture> load_texture(
            std::string path_texture,
            Texture_Loaded_Callback callback = nullptr
        );

        std::shared_ptr<Mesh> load_mesh(
            std::string path_mesh,
            Mesh_Loaded_Callback callback = nullptr
        );

        void destroy_data();

        ~Assets_Manager();
    };
    
}