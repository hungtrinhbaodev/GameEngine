#pragma once
#include <vulkan/vulkan.h>

#include <functional>

#include <core/resource.hpp>
#include <graphic/common/texture.h>
#include <graphic/vulkan_implementation/vulkan_image.h>
#include <graphic/vulkan_implementation/vulkan_queues.h>
#include <graphic/vulkan_implementation/vulkan_command_pool.h>

namespace Graphic {

    struct Vulkan_Texture_Load_Description {

        Core::Resource_Load_Mode load_mode = Core::Resource_Load_Mode::SYNC;

        std::shared_ptr<Texture> texture = nullptr;

        std::function<void()> callback = nullptr;

    };

    class Vulkan_Texture : public Core::Resource<Vulkan_Texture_Load_Description, std::string> {

        private:

        Vulkan_Image _vk_image;

        VkSampler _vk_sampler;

        void _make_vk_sampler(VkDevice vk_device, VkPhysicalDevice vk_physical_device);

        public:

        void on_load(const Vulkan_Texture_Load_Description& des);

        void on_finish_load();

        VkImageView get_vk_imageview();

        VkSampler get_vk_sampler();

        void destroy(VkDevice vk_device);

    };

}
