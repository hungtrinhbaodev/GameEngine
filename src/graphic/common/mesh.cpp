#include <graphic/common/mesh.h>

void Graphic::Mesh::on_load(const Mesh_Load_Params& params) {
    const std::string& path = params.path;
    // Some hardcode to render here
    if (path == "TRIANGLE") {
        _vertices = {
            {{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f}, {}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, -0.5f, 0.0f}, {0.5f, 0.0f}, {}, {0.0f, 1.0f, 0.0f}},
            {{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}, {}, {0.0f, 0.0f, 1.0f}}
        };
        _indices = {
            0, 1, 2
        };
    }
    else if (path == "RECTANGLE") {
        _vertices = {
            {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, -0.5f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.1f}},
            {{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.1f}},
            {{-0.5f, 0.5f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 1.0f}}
        };
        _indices = {
            0, 1, 2, 2, 3, 0
        };
    }
    else if (path == "CUBE") {

    }
}

void* Graphic::Mesh::get_vertices_data() {
    return _vertices.data();
}

size_t Graphic::Mesh::get_vertices_memory_size() {
    return sizeof(Vertex) * _vertices.size();
}

void* Graphic::Mesh::get_indices_data() {
    return _indices.data();
}

size_t Graphic::Mesh::get_indices_memory_size() {
    return sizeof(uint16_t) * _indices.size(); 
}