#pragma once
#include <vector>
#include <string>
#include <stdexcept>

#include <core/resource.hpp>
#include <graphic/common/vertex.h>

namespace Graphic {

    struct Mesh_Load_Params {
        std::string path;
    };
    
    class Mesh : public Core::Resource<Mesh_Load_Params, std::string> {

        private:

        std::vector<Vertex> _vertices;

        std::vector<uint16_t> _indices;

        public:

        void on_load(const Mesh_Load_Params& params);

        const std::vector<Vertex>& get_vertices();

        void* get_vertices_data();

        uint32_t get_vertices_memory_size();

        void* get_indices_data();

        uint32_t get_indices_memory_size();
    };

};
