#pragma once
#include <vulkan/vulkan.h>

#include <functional>

#include <graphic/common/assets_manager.h>
#include <graphic/vulkan_implementation/vulkan_descriptor.h>
#include <graphic/vulkan_implementation/vulkan_mesh_buffer.hpp>
#include <graphic/vulkan_implementation/vulkan_instances_buffer.hpp>
#include <graphic/vulkan_implementation/vulkan_texture.h>
#include <graphic/vulkan_implementation/vulkan_model.h>
#include <utility/log_utils.h>

namespace Graphic {

    using Mesh_Buffer = Vulkan_Mesh_Buffer<std::string>;

    using Instance_Buffer = Vulkan_Instances_Buffer<long, glm::mat4>;

    class Vulkan_Assets_Manager : public Assets_Manager {

        private:

        Core::Resource_Storage<std::string, Vulkan_Texture, Vulkan_Texture_Load_Description>* _vk_texs_storage = nullptr;

        Core::Resource_Storage<std::string, Vulkan_Mesh, Vulkan_Mesh_Load_Params>* _vk_mesh_storage = nullptr;

        Core::Resource_Storage<std::string, Vulkan_Model, Vulkan_Model_Load_Params>* _vk_model_storage = nullptr;

        Vulkan_Descriptor* _wp_descriptor;

        public:

        Vulkan_Assets_Manager();

        void init_data(Vulkan_Descriptor* wp_descriptor);

        std::shared_ptr<Vulkan_Texture> load_vk_texture(
            Core::Resource_Load_Mode load_mode,
            std::string path
        );

        std::shared_ptr<Vulkan_Mesh> load_vk_mesh(
            Core::Resource_Load_Mode load_mode,
            std::string path,
            Mesh_Buffer* vk_vertices_buffer,
            Mesh_Buffer* vk_indices_buffer
        );

        std::shared_ptr<Vulkan_Model> load_vk_model(
            Core::Resource_Load_Mode load_mode,
            std::string path_mesh,
            std::string path_texture,
            Mesh_Buffer* vk_vertices_buffer,
            Mesh_Buffer* vk_indices_buffer
        );

        void destroy_data(VkDevice vk_device);

        ~Vulkan_Assets_Manager();
    };

}