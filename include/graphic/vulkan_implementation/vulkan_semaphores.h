#pragma once
#include <core/concurent_pool.hpp>
#include <graphic/vulkan_implementation/vulkan_utility.h>

namespace Graphic {

    class Vulkan_Semaphores : public Core::Concurent_Pool<VkSemaphore> {

        private:

        VkDevice _vk_device = VK_NULL_HANDLE;

        public:

        void init(VkDevice vk_device);

        VkSemaphore _create_item();

        void _delete_item(VkSemaphore& vk_semaphore);

    };

};