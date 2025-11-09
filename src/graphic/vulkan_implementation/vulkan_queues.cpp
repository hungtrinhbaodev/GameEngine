#include <graphic/vulkan_implementation/vulkan_queues.h>
#include <graphic/vulkan_implementation/vulkan_utility.h>
#include <utility/time_utils.h>
#include <graphic/common/graphic_constants.h>

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
    Utility::Log::get()->log_info("Get device graphic queue at index",  indices.graphic_family.value(), _vk_graphics_queue, "success!");

    vkGetDeviceQueue(vk_device, indices.present_family.value(), 0, &_vk_present_queue);
    Utility::Log::get()->log_info("Get device present queue at index", indices.present_family.value(), _vk_present_queue, "success!");
}

void Graphic::Vulkan_Queues::submit_single_commands(
    Graphic::Vulkan_Queue_Submit_Mode submit_mode, 
    VkCommandBuffer command_buffer,
    Graphic::Submit_Callback callback
) {

    std::unique_lock<std::mutex> lock(_sumit_lock);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    // Utility::Log::get()->log_info("submit_single_commands 1", "command buffer:", command_buffer);

    switch(submit_mode) {
        case Vulkan_Queue_Submit_Mode::SUBMIT_MODE_SYNC: {
            // Utility::Log::get()->log_info("submit_single_commands 2", "command buffer:", command_buffer, "graphic queue: ", _vk_graphics_queue);

            Utility::Time_Utils::get()->start_track(Graphic_Constants::KEY_TRACK_TIME_QUEUE_SEND_COMMONS);
            vkQueueSubmit(_vk_graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
            Utility::Time_Utils::get()->end_track(Graphic_Constants::KEY_TRACK_TIME_QUEUE_SEND_COMMONS);

            // Utility::Log::get()->log_info("submit_single_commands 2.1", "graphic queue: ", _vk_graphics_queue);
            vkQueueWaitIdle(_vk_graphics_queue);
            
            // Utility::Log::get()->log_info("submit_single_commands 3");
            lock.unlock();

            if(callback != nullptr) {
                callback();
            }
            // Utility::Log::get()->log_info("submit_single_commands 4");
            break;
        }
        default: {
            // Utility::Log::get()->log_info("submit_single_commands 3");
            _vk_fences->using_fence_with_callback(
                [this, submit_info](VkFence vk_fence) {
                    // Utility::Log::get()->log_info("submit_single_commands 4", vk_fence);
                    Utility::Time_Utils::get()->start_track(Graphic_Constants::KEY_TRACK_TIME_QUEUE_SEND_COMMONS);
                    vkQueueSubmit(_vk_graphics_queue, 1, &submit_info, vk_fence);
                    Utility::Time_Utils::get()->end_track(Graphic_Constants::KEY_TRACK_TIME_QUEUE_SEND_COMMONS);
                    // Utility::Log::get()->log_info("submit_single_commands 5", vkGetFenceStatus(_vk_device, vk_fence));
                },
                callback
            );
            break;
        }
    }
}

void Graphic::Vulkan_Queues::submit_custom_commands(
    const VkSubmitInfo& vk_submit_info,
    VkFence vk_fence
) {
    std::unique_lock<std::mutex> lock(_sumit_lock);
    Utility::Time_Utils::get()->start_track(Graphic_Constants::KEY_TRACK_TIME_QUEUE_SEND_DRAW);
    vkQueueSubmit(_vk_graphics_queue, 1, &vk_submit_info, vk_fence);
    Utility::Time_Utils::get()->end_track(Graphic_Constants::KEY_TRACK_TIME_QUEUE_SEND_DRAW);
}

void Graphic::Vulkan_Queues::submit_present_commands(
    const VkPresentInfoKHR& vk_present_info
) {
    std::unique_lock<std::mutex> lock(_sumit_lock);
    Vulkan_Utility::vk_check_action(
        vkQueuePresentKHR(_vk_present_queue, &vk_present_info),
        "failed to submit present command buffer!"
    );
}

VkQueue Graphic::Vulkan_Queues::get_graphics_queue() {
    return _vk_graphics_queue;
}

VkQueue Graphic::Vulkan_Queues::get_present_queue() {
    return _vk_present_queue;
}

void Graphic::Vulkan_Queues::wait_to_idle() {
    vkQueueWaitIdle(_vk_graphics_queue);
}