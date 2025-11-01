#pragma once

#include <string>
#include <stdexcept>

#include <core/resource.hpp>
#include <graphic/common/mesh.h>
#include <graphic/vulkan_implementation/vulkan_mesh_buffer.hpp>

namespace Graphic {

    struct Vulkan_Mesh_Load_Params {

        std::shared_ptr<Mesh> mesh;

        Vulkan_Mesh_Buffer<std::string>* vertices_buffer = nullptr;

        Vulkan_Mesh_Buffer<std::string>* indices_buffer = nullptr;

    };

    class Vulkan_Mesh : public Core::Resource<Vulkan_Mesh_Load_Params, std::string> {

        public:

        void on_load(const Vulkan_Mesh_Load_Params& load_param);

    };

}

