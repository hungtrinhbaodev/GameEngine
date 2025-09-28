#pragma once
#include <vulkan/vulkan.h>

#include <functional>

#include <graphic/common/assets_manager.h>
#include <graphic/vulkan_implementation/vulkan_texture.h>
#include <utility/log_utils.h>

namespace Graphic {

    class Vulkan_Assets_Manager : public Assets_Manager {

        private:

        Vulkan_Texture_Storage* _vk_texs_stroage;

        public:

        Vulkan_Assets_Manager();

        void init_data();

        Vulkan_Texture* load_vk_texture(
            std::string path,
            VkDevice vk_device = VK_NULL_HANDLE,
            VkPhysicalDevice vk_physical_device = VK_NULL_HANDLE,
            Vulkan_Queues* vk_queues = nullptr,
            Vulkan_Command_Pool* vk_command_pool = nullptr
        );

        void destroy_data(VkDevice vk_device);

        ~Vulkan_Assets_Manager();

    };

}