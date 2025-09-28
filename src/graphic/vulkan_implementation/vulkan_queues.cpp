#include <graphic/vulkan_implementation/vulkan_queues.h>
#include <graphic/vulkan_implementation/vulkan_utility.h>

void Graphic::Vulkan_Queues::init_queues(
    VkPhysicalDevice vk_physical_device,
    VkSurfaceKHR vk_surface,
    VkDevice vk_device,
    Vulkan_Fences* vk_fences
) {
    _vk_fences = vk_fences;

    _vk_device = vk_device;

    Vulkan_Queue_Family_Indices indices = Vulkan_Utility::query_suitable_queue_family_indices(
        vk_physical_device,
        vk_surface
    );

    vkGetDeviceQueue(vk_device, indices.graphic_family.value(), 0, &_vk_graphics_queue);
    Utility::Log::get()->log_info("Get device graphic queue at index",  indices.graphic_family.value(), "success!");

    vkGetDeviceQueue(vk_device, indices.present_family.value(), 0, &_vk_present_queue);
    Utility::Log::get()->log_info("Get device present queue at index", indices.present_family.value(), "success!");
}

void Graphic::Vulkan_Queues::submit_single_commands(
    Graphic::Vulkan_Queue_Submit_Mode submit_mode, 
    VkCommandBuffer command_buffer,
    void* user_data,
    Graphic::Submit_Callback callback
) {

    _sumit_mutex.lock();

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    Utility::Log::get()->log_info("submit_single_commands 1");

    switch(submit_mode) {
        case Vulkan_Queue_Submit_Mode::SUBMIT_MODE_SYNC: {
            Utility::Log::get()->log_info("submit_single_commands 2");
            vkQueueSubmit(_vk_graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
            vkQueueWaitIdle(_vk_graphics_queue);
            break;
        }
        default: {
            Utility::Log::get()->log_info("submit_single_commands 3");
            _vk_fences->using_fence_with_callback(
                user_data,
                [&](VkFence vk_fence) {
                    Utility::Log::get()->log_info("submit_single_commands 4", vk_fence);
                    vkQueueSubmit(_vk_graphics_queue, 1, &submit_info, vk_fence);
                    Utility::Log::get()->log_info("submit_single_commands 5", vkGetFenceStatus(_vk_device, vk_fence));
                },
                callback
            );
            break;
        }
    }

    _sumit_mutex.unlock();
}

VkQueue Graphic::Vulkan_Queues::get_graphics_queue() {
    return _vk_graphics_queue;
}

VkQueue Graphic::Vulkan_Queues::get_present_queue() {
    return _vk_present_queue;
}