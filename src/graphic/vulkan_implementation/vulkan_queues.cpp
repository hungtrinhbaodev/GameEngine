#include <graphic/vulkan_implementation/vulkan_queues.h>

void Graphic::Vulkan_Queues::init_queues(
    VkPhysicalDevice vk_physical_device,
    VkSurfaceKHR vk_surface,
    VkDevice vk_device
) {
    Vulkan_Queue_Family_Indices indices = Vulkan_Utility::query_suitable_queue_family_indices(
        vk_physical_device,
        vk_surface
    );

    vkGetDeviceQueue(vk_device, indices.graphic_family.value(), 0, &_vk_graphics_queue);
    Utility::Log::get()->log_info("Get device graphic queue at index",  indices.graphic_family.value(), "success!");

    vkGetDeviceQueue(vk_device, indices.present_family.value(), 0, &_vk_present_queue);
    Utility::Log::get()->log_info("Get device present queue at index", indices.present_family.value(), "success!");
}

VkQueue Graphic::Vulkan_Queues::get_graphics_queue() {
    return _vk_graphics_queue;
}

VkQueue Graphic::Vulkan_Queues::get_present_queue() {
    return _vk_present_queue;
}