#include <graphic/vulkan_implementation/vulkan_semaphores.h>

void Graphic::Vulkan_Semaphores::init(VkDevice vk_device) {
    _vk_device = vk_device;
}

VkSemaphore Graphic::Vulkan_Semaphores::_create_item() {
    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkSemaphore vk_semaphore;
    vkCreateSemaphore(
        _vk_device,
        &semaphore_info,
        nullptr,
        &vk_semaphore
    );
    return vk_semaphore;
}

void Graphic::Vulkan_Semaphores::_delete_item(VkSemaphore& vk_semaphore) {
    vkDestroySemaphore(_vk_device, vk_semaphore, nullptr);
}