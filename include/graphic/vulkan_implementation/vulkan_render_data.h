#pragma once

#include <map>
#include <mutex>

#include <core/objects_id_generated.hpp>
#include <graphic/vulkan_implementation/vulkan_constants.h>
#include <graphic/vulkan_implementation/vulkan_assets_manager.h>
#include <graphic/vulkan_implementation/vulkan_model.h>
#include <graphic/vulkan_implementation/vulkan_mesh_buffer.hpp>
#include <graphic/vulkan_implementation/vulkan_instances_buffer.hpp>
#include <graphic/vulkan_implementation/vulkan_swapchain.h>
#include <graphic/vulkan_implementation/vulkan_fences.h>
#include <graphic/vulkan_implementation/vulkan_semaphores.h>

namespace Graphic {

    struct Vulkan_Model_User_Data {

        std::string model_key;

        long model_id;

        std::shared_ptr<Vulkan_Model> vk_model;
        
    };

    class Vulkan_Render_Data {

        private:

        Vulkan_Assets_Manager* _vk_asset_mgr = nullptr;

        std::mutex _models_lock;

        std::map<std::string, std::shared_ptr<Vulkan_Model>> _vk_models;

        Mesh_Buffer* _vk_vertices_buffer;

        Mesh_Buffer* _vk_indices_buffer;

        std::map<std::string, Instance_Buffer*> _vk_instance_buffers;

        Core::Longs_ID_Generated _id_generate;

        public:

        Vulkan_Render_Data();

        void init(
            Vulkan_Assets_Manager* vk_asset_mgr
        );

        Vulkan_Model_User_Data add_model(
            std::string mesh_path, 
            std::string texture_path,
            glm::mat4 transform
        );

        void update_model(const Vulkan_Model_User_Data& model_data, glm::mat4 transform);

        void delete_model(const Vulkan_Model_User_Data& model_data);

        Mesh_Buffer* get_vertices_buffer();

        Mesh_Buffer* get_indices_buffer();

        Instance_Buffer* get_instances_buffer(std::string key);

        /**
         * Model will be sort by draw id
         * and texture to drawing proccess
         * is optimal
         */
        std::map<Vulkan_Draw_ID, std::map<std::string, std::vector<std::shared_ptr<Vulkan_Model>>>> get_render_models();

        void destroy();

        ~Vulkan_Render_Data();

    };

}