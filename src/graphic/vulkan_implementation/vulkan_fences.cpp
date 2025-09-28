#include <graphic/vulkan_implementation/vulkan_fences.h>
#include <graphic/vulkan_implementation/vulkan_utility.h>

void Graphic::Vulkan_Fences::init(VkDevice vk_device) {
    _vk_device = vk_device;
}

VkFence Graphic::Vulkan_Fences::_create_item() {

    VkFenceCreateInfo vk_fence_info{};
    vk_fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vk_fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VkFence vk_fence;
    Vulkan_Utility::vk_check_action(
        vkCreateFence(_vk_device, &vk_fence_info, nullptr, &vk_fence),
        "fail to create fence"
    );

    return vk_fence;
}

void Graphic::Vulkan_Fences::pooling_item(const VkFence& item) {
    Utility::Log::get()->log_info("pooling_item fence", item);
    vkResetFences(_vk_device, 1, &item);
    Utility::Log::get()->log_info("pooling_item reset fence", item);
    Core::Concurent_Pool<VkFence>::pooling_item(item);
}

void Graphic::Vulkan_Fences::_delete_item(VkFence& vk_fence) {
    vkDestroyFence(_vk_device, vk_fence, nullptr);
}

void Graphic::Vulkan_Fences::using_fence_with_callback(
    void* user_data,
    Using_Fence using_fence,
    Graphic::Fence_Success_Callback callback
) {

    Utility::Log::get()->log_info("using_fence_with_callback 1", _pool.size());

    VkFence& vk_fence = request_item();

    // Utility::Log::get()->log_info("request_item_with_callback 2", _pool.size());

    _callback_lock.lock();

    if (_vk_fence_callbacks.find(vk_fence) != _vk_fence_callbacks.end()) {
        Vulkan_Utility::vk_check_action(
            VK_INCOMPLETE,
            "Fence request with callback is using before!"
        );
    }
    _vk_fence_callbacks[vk_fence] = callback;
    _user_data_callbacks[vk_fence] = user_data;

    Utility::Log::get()->log_info("using_fence_with_callback 2", vk_fence, vkGetFenceStatus(_vk_device, vk_fence));
    
    vkResetFences(_vk_device, 1, &vk_fence);

    using_fence(vk_fence);

    // Utility::Log::get()->log_info("request_item_with_callback 3", _vk_fence_callbacks.size());

    _callback_lock.unlock();
}

void Graphic::Vulkan_Fences::update_data() {
    _callback_lock.lock();

    std::vector<VkFence> remove_fences;
    if (_vk_fence_callbacks.size() > 0) {
        // Utility::Log::get()->log_info("Vulkan_Fences::update_data 1", _vk_fence_callbacks.size());
    }
    for (const auto& [vk_fence, callback] : _vk_fence_callbacks) {
        if (_vk_fence_callbacks.size() > 0) {
            // Utility::Log::get()->log_info("Vulkan_Fences::update_data 2", _vk_fence_callbacks.size(), vkGetFenceStatus(_vk_device, vk_fence) == VK_SUCCESS ? "true" : "false");
        }
        if (vkGetFenceStatus(_vk_device, vk_fence) == VK_SUCCESS) {
            void* user_data = _user_data_callbacks[vk_fence];
            if (callback != nullptr) {
                std::thread t(callback, user_data);
                t.detach();
            }
            if (_vk_fence_callbacks.size() > 0) {
                // Utility::Log::get()->log_info("Vulkan_Fences::update_data 3", _vk_fence_callbacks.size(), vkGetFenceStatus(_vk_device, vk_fence) == VK_SUCCESS ? "true" : "false");
            }
            remove_fences.push_back(vk_fence);
        }
    }
    if (remove_fences.size() > 0) {
        Utility::Log::get()->log_info("Vulkan_Fences::update_data 4", remove_fences.size());
    }
    for (const auto& vk_fence : remove_fences) {
        _vk_fence_callbacks.erase(vk_fence);
        _user_data_callbacks.erase(vk_fence);
        pooling_item(vk_fence);
    }

    _callback_lock.unlock();
}

Graphic::Vulkan_Fences::~Vulkan_Fences() {
    
}
