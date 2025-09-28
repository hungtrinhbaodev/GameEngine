#pragma once
#include <vulkan/vulkan.h>

#include <vector>
#include <map>
#include <mutex>
#include <thread>

#include <core/concurent_pool.hpp>
#include <graphic/vulkan_implementation/vulkan_constants.h>
#include <utility/log_utils.h>

namespace Graphic {

    using Fence_Success_Callback = std::function<void(void*)>;

    using Using_Fence = std::function<void(VkFence)>;

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

        void pooling_item(const VkFence& item);

        void using_fence_with_callback(
            void* user_data,
            Using_Fence using_fence,
            Fence_Success_Callback callback
        );

        void update_data();

        ~Vulkan_Fences();
    };
}