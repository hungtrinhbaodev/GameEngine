#include <graphic/vulkan_implementation/vulkan_fences.h>
#include <graphic/vulkan_implementation/vulkan_utility.h>

/**
 * Thread_Callback_Fence_Item field
 */

void Graphic::Thread_Callback_Fence_Item::do_task(Task_Callback_Using_Fence task_info) {
    const auto& callback = task_info.callback;
    if (callback != nullptr) {
        callback();
    }
}

/**
 * Vulkan_Fences field
 */

void Graphic::Vulkan_Fences::init(VkDevice vk_device) {
    _vk_device = vk_device;
    _callback_thread_pool.start_running(10);
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
    vkResetFences(_vk_device, 1, &item);
    Core::Concurent_Pool<VkFence>::pooling_item(item);
}

void Graphic::Vulkan_Fences::_delete_item(VkFence& vk_fence) {
    vkDestroyFence(_vk_device, vk_fence, nullptr);
}

void Graphic::Vulkan_Fences::using_fence_with_callback(
    Using_Fence using_fence,
    Graphic::Fence_Success_Callback callback
) {

    std::unique_lock<std::mutex> lock(_callback_lock);

    VkFence& vk_fence = request_item();

    if (_vk_fence_callbacks.find(vk_fence) != _vk_fence_callbacks.end()) {
        Vulkan_Utility::vk_check_action(
            VK_INCOMPLETE,
            "Fence request with callback is using before!"
        );
    }
    _vk_fence_callbacks[vk_fence] = callback;
    
    vkResetFences(_vk_device, 1, &vk_fence);

    using_fence(vk_fence);

    _callback_lock.unlock();
}

void Graphic::Vulkan_Fences::update_data() {

    std::unique_lock<std::mutex> lock(_callback_lock);

    std::vector<VkFence> remove_fences;

    for (const auto& [vk_fence, callback] : _vk_fence_callbacks) {
        if (vkGetFenceStatus(_vk_device, vk_fence) == VK_SUCCESS) {
            _callback_thread_pool.push_task(Task_Callback_Using_Fence {
                callback
            });
            remove_fences.push_back(vk_fence);
        }
    }

    for (const auto& vk_fence : remove_fences) {
        _vk_fence_callbacks.erase(vk_fence);
        pooling_item(vk_fence);
    }
}

void Graphic::Vulkan_Fences::destroy() {
    _callback_thread_pool.destroy();
    Core::Concurent_Pool<VkFence>::destroy();
}

Graphic::Vulkan_Fences::~Vulkan_Fences() {
    
}
