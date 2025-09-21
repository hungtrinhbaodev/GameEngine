#pragma once
#include <graphic/common/texture.h>
#include <graphic/common/vertex.h>
#include <string>
#include <vector>

namespace Graphic {

    enum ModelLoadedState {
        UNLOAD = 0,
        LOADING = 1,
        LOADED = 2
    };

    class Model {

        private:

        ModelLoadedState loaded_state;

        std::vector<Vertex> verts;

        std::vector<int> indices;

        std::vector<Texture *> textures;

        public:

        void load_model(std::string path);
    };

}