#include <graphic/vulkan_implementation/vulkan_command_pool.h>
#include <graphic/vulkan_implementation/vulkan_core_data.h>
#include <graphic/vulkan_implementation/vulkan_utility.h>

VkCommandBuffer Graphic::Vulkan_Command_Pool::_create_item() {

    Utility::Log::get()->log_info("Vulkan_Command_Pool::_create_item 1");

    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandPool = _vk_command_pool;
    allocate_info.commandBufferCount = 1;

    Utility::Log::get()->log_info("Vulkan_Command_Pool::_create_item 2");

    VkCommandBuffer command_buffer{};
    vkAllocateCommandBuffers(_vk_device, &allocate_info, &command_buffer);

    Utility::Log::get()->log_info("Vulkan_Command_Pool::_create_item 3");
    
    return command_buffer;
}

void Graphic::Vulkan_Command_Pool::pooling_item(const VkCommandBuffer& command_buffer) {
    vkResetCommandBuffer(command_buffer, 0);
    Core::Concurent_Pool<VkCommandBuffer>::pooling_item(command_buffer);
}

void Graphic::Vulkan_Command_Pool::_destroy_item(VkCommandBuffer &command_buffer) {
    vkFreeCommandBuffers(_vk_device, _vk_command_pool, 1, &command_buffer);
}

void Graphic::Vulkan_Command_Pool::init(
    VkPhysicalDevice vk_physical_device, 
    VkDevice vk_device, 
    VkSurfaceKHR vk_surface
) {
    _vk_device = vk_device;

    Vulkan_Queue_Family_Indices indices = Vulkan_Utility::query_suitable_queue_family_indices(
        vk_physical_device,
        vk_surface
    );

    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = indices.graphic_family.value();

    // init vulkan command pool
    Vulkan_Utility::vk_check_action(
        vkCreateCommandPool(vk_device, &pool_info, nullptr, &_vk_command_pool),
        "failed to create command pool!"
    );
    Utility::Log::get()->log_info("Create command pool successfully!");

    // init vulkan draw commands
    for (int i = 0; i < Vulkan_Constants::MAX_FRAMES_IN_FLIGHT; i++) {
        _vk_draw_command_buffers.push_back(request_item());
    }
}

void Graphic::Vulkan_Command_Pool::record_single_commands(
    Vulkan_Commands_Mode commands_mode,
    Graphic::CommandRecord record,
    void* user_data,
    Graphic::CommandCallback callback,
    Graphic::Vulkan_Queues* queues
) {

    Utility::Log::get()->log_info("record_single_commands 1");
    if (queues == nullptr) {
        const auto& wp_data = Vulkan_Core_Data::get()->get_wrapper_data();
        queues = wp_data.wp_queues;
    }

    if (queues == nullptr) {
        Vulkan_Utility::vk_check_action(
            VK_INCOMPLETE,
            "Fail to send command: not found any Vulkan_Queues!"
        );
        return;
    }
    Utility::Log::get()->log_info("record_single_commands 2");

    VkCommandBuffer& command_buffer = request_item();

    Utility::Log::get()->log_info("record_single_commands 3");

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);
        record(command_buffer);
    vkEndCommandBuffer(command_buffer);

    Utility::Log::get()->log_info("record_single_commands 4", commands_mode == Vulkan_Commands_Mode::COMMANDS_MODE_SYNC ? "sync" : "async");

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    switch(commands_mode) {
        case Vulkan_Commands_Mode::COMMANDS_MODE_SYNC: {
            Utility::Log::get()->log_info("record_single_commands 5");
            queues->submit_single_commands(
                Vulkan_Queue_Submit_Mode::SUBMIT_MODE_SYNC,
                command_buffer
            );
            break;
        }
        default: {
            queues->submit_single_commands(
                Vulkan_Queue_Submit_Mode::SUBMIT_MODE_ASYNC,
                command_buffer,
                user_data,
                [&, callback] (void* user_data){
                    Utility::Log::get()->log_info("record_single_commands 6");    
                    pooling_item(command_buffer);
                    Utility::Log::get()->log_info("record_single_commands 7");   
                    if(callback != nullptr) {
                        callback(user_data);
                    }
                    Utility::Log::get()->log_info("record_single_commands 8");   
                }
            );
            break;
        }
    }
}

void Graphic::Vulkan_Command_Pool::destroy() {   

    // destroy all command buffer is created
    Core::Concurent_Pool<VkCommandBuffer>::destroy();

    // destroy command pool
    vkDestroyCommandPool(_vk_device, _vk_command_pool, nullptr);
    Utility::Log::get()->log_info("Destroy command pool success!");
}