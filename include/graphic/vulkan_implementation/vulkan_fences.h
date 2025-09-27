#pragma once
#include <vulkan/vulkan.h>

#include <vector>
#include <map>
#include <mutex>

#include <core/concurent_pool.hpp>
#include <graphic/vulkan_implementation/vulkan_utility.h>
#include <graphic/vulkan_implementation/vulkan_constants.h>
#include <utility/log_utils.h>

namespace Graphic {

    using Fence_Success_Callback = std::function<void(void*)>;

    class Vulkan_Fences : public Core::Concurent_Pool<VkFence> {

        private:

        VkDevice _vk_device;

        std::mutex _callback_lock;

        std::map<VkFence, Fence_Success_Callback> _vk_fence_callbacks;

        std::map<VkFence, void*> _user_data_callbacks;

        VkFence _create_item();

        void _delete_item(VkFence& vk_fence);

        public:

        void init(
            VkDevice vk_device
        );

        VkFence& request_item_with_callback(
            void* user_data,
            Fence_Success_Callback callback
        );

        void update_data();

        ~Vulkan_Fences();
    };
}