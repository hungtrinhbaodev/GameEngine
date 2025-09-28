#pragma once
#include <vulkan/vulkan.h>

#include <core/resource.hpp>
#include <core/resource_storage.hpp>
#include <graphic/common/texture.h>
#include <graphic/vulkan_implementation/vulkan_image.h>
#include <graphic/vulkan_implementation/vulkan_queues.h>
#include <graphic/vulkan_implementation/vulkan_command_pool.h>

namespace Graphic {

    struct Vulkan_Texture_Load_Description {

        Texture *texture = nullptr;

        VkPhysicalDevice vk_physical_device = VK_NULL_HANDLE;

        VkDevice vk_device = VK_NULL_HANDLE;

        Vulkan_Queues* wp_queues = nullptr;

        Vulkan_Command_Pool* wp_command_pool = nullptr;

    };

    class Vulkan_Texture : public Core::Resource {

        private:

        Vulkan_Image _vk_image;

        VkSampler _vk_sampler;

        void _make_vk_sampler(VkDevice vk_device, VkPhysicalDevice vk_physical_device);

        public:

        void load_vk_texture(const Vulkan_Texture_Load_Description& des);

        void on_resource_loaded_finish();

        void destroy(VkDevice vk_device);

    };

    class Vulkan_Texture_Storage : public Core::Resource_Storage<std::string, Vulkan_Texture, Vulkan_Texture_Load_Description, Vulkan_Texture_Storage> {

        public:

        void _load_resource(
            Vulkan_Texture* vk_texture,
            const Vulkan_Texture_Load_Description& description
        );

        void destroy_resources(VkDevice vk_device);

    };

}
