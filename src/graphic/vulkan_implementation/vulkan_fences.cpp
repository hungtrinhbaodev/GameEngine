#include <graphic/vulkan_implementation/vulkan_fences.h>

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

void Graphic::Vulkan_Fences::_delete_item(VkFence& vk_fence) {
    vkDestroyFence(_vk_device, vk_fence, nullptr);
}

VkFence& Graphic::Vulkan_Fences::request_item_with_callback(
    void* user_data,
    Graphic::Fence_Success_Callback callback
) {

    VkFence& vk_fence = request_item();

    _callback_lock.lock();

    if(_vk_fence_callbacks.find(vk_fence) != _vk_fence_callbacks.end()) {
        Vulkan_Utility::vk_check_action(
            VK_INCOMPLETE,
            "Fence request with callback is using before!"
        );
    }
    _vk_fence_callbacks[vk_fence] = callback;
    _user_data_callbacks[vk_fence] = user_data;

    _callback_lock.unlock();

    return vk_fence;
}

void Graphic::Vulkan_Fences::update_data() {
    std::vector<VkFence> remove_fences;
    for (const auto& [vk_fence, callback] : _vk_fence_callbacks) {
        if (vkGetFenceStatus(_vk_device, vk_fence) == VK_SUCCESS) {
            void* user_data = _user_data_callbacks[vk_fence];
            callback(user_data);
            remove_fences.push_back(vk_fence);
        }
    }
    for (const auto& vk_fence : remove_fences) {
        _vk_fence_callbacks.erase(vk_fence);
        _user_data_callbacks.erase(vk_fence);
        pooling_item(vk_fence);
    }
}

Graphic::Vulkan_Fences::~Vulkan_Fences() {
    
}
