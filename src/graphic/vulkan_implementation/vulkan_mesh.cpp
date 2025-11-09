#include <graphic/vulkan_implementation/vulkan_mesh.h>

void Graphic::Vulkan_Mesh::on_load(const Vulkan_Mesh_Load_Params& load_param) {

    if (load_param.vertices_buffer == nullptr || load_param.indices_buffer == nullptr) {
        throw std::runtime_error("Fail to load vulkan mesh into buffer: wrong buffer param!");
    }

    const auto& mesh = load_param.mesh;
    auto vertices_buffer = load_param.vertices_buffer;
    auto indices_buffer = load_param.indices_buffer;
    
    // TODO: set up event driven or callback to 
    // make push vertices to buffer is async task
    vertices_buffer->push_data(
        mesh->get_key(), 
        mesh->get_vertices_data(), 
        0,
        mesh->get_vertices_memory_size()
    );

    indices_buffer->push_data(
        mesh->get_key(),
        mesh->get_indices_data(),
        0,
        mesh->get_indices_memory_size()
    );
}