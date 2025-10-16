#include <graphic/vulkan_implementation/vulkan_command_pool.h>
#include <graphic/vulkan_implementation/vulkan_core_data.h>
#include <graphic/vulkan_implementation/vulkan_utility.h>

/**
 * Vulkan_Command_Buffer_Pool field
 */

VkCommandBuffer Graphic::Vulkan_Command_Buffer_Pool::_create_item() {
    VkCommandBuffer vk_command_buffer = VK_NULL_HANDLE;
    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandPool = _vk_command_pool;
    allocate_info.commandBufferCount = 1;

    vkAllocateCommandBuffers(_vk_device, &allocate_info, &vk_command_buffer);
    return vk_command_buffer;
}

void Graphic::Vulkan_Command_Buffer_Pool::_delete_item(VkCommandBuffer& item) {
    // free command buffer in thread
    vkFreeCommandBuffers(_vk_device, _vk_command_pool, 1, &item);
}

void Graphic::Vulkan_Command_Buffer_Pool::init(
    VkDevice vk_device,
    VkCommandPool vk_command_pool
) {
    _vk_device = vk_device;
    _vk_command_pool = vk_command_pool;
}

void Graphic::Vulkan_Command_Buffer_Pool::pooling_item(const VkCommandBuffer& item) {
    vkResetCommandBuffer(item, 0);
}

/**
 * Command_Thread_Item field
 */

void Graphic::Vulkan_Command_Thread_Item::init(
    VkPhysicalDevice vk_physical_device,
    VkDevice vk_device, 
    VkSurfaceKHR vk_surface, 
    Vulkan_Queues* wp_queues
) {

    _wp_queues = wp_queues;
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

    _wp_command_buffer_pool.init(
        _vk_device,
        _vk_command_pool
    );
}

void Graphic::Vulkan_Command_Thread_Item::destroy() {

    // destroy all command buffer is request in thread
    _wp_command_buffer_pool.destroy();

    // destroy command pool in thread
    vkDestroyCommandPool(_vk_device, _vk_command_pool, nullptr);
    // Utility::Log::get()->log_info("Destroy command pool success!");

}

Graphic::Vulkan_Command_Thread_Item::~Vulkan_Command_Thread_Item() {

}

void Graphic::Vulkan_Command_Thread_Item::do_task(Vulkan_Command_Task_Info task_info) {
    Utility::Log::get()->log_info("record_single_commands 4", task_info.commands_mode);
    if (_wp_queues == nullptr) {
        const auto& wp_data = Vulkan_Core_Data::get()->get_wrapper_data();
        _wp_queues = wp_data.wp_queues;
    }

    if (_wp_queues == nullptr) {
        Vulkan_Utility::vk_check_action(
            VK_INCOMPLETE,
            "Fail to send command: not found any Vulkan_Queues!"
        );
        return;
    }

    Utility::Log::get()->log_info("record_single_commands 5");

    VkCommandBuffer command_buffer = _wp_command_buffer_pool.request_item();

    Utility::Log::get()->log_info("record_single_commands 6", "command buffer:", command_buffer);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);
        Utility::Log::get()->log_info("record_single_commands 7", "record", (void*)(&task_info.record));
        task_info.record(command_buffer);
        Utility::Log::get()->log_info("record_single_commands 8", "command buffer:", command_buffer);
    vkEndCommandBuffer(command_buffer);

    Utility::Log::get()->log_info("record_single_commands 9");
    switch(task_info.commands_mode) {
        case Vulkan_Commands_Mode::COMMANDS_MODE_SYNC: {
            Utility::Log::get()->log_info("record_single_commands 10");
            _wp_queues->submit_single_commands(
                Vulkan_Queue_Submit_Mode::SUBMIT_MODE_SYNC,
                command_buffer,
                task_info.callback
            );
            break;
        }
        default: {
            Utility::Log::get()->log_info("record_single_commands 11");
            _wp_queues->submit_single_commands(
                Vulkan_Queue_Submit_Mode::SUBMIT_MODE_ASYNC,
                command_buffer,
                [this, task_info, command_buffer] (){  
                    if(task_info.callback != nullptr) {
                        task_info.callback();
                    }
                    _wp_command_buffer_pool.pooling_item(command_buffer);
                }
            );
            break;
        }
    }
}

Graphic::Vulkan_Command_Thread_Pool::Vulkan_Command_Thread_Pool(
    VkPhysicalDevice vk_physical_device,
    VkDevice vk_device, 
    VkSurfaceKHR vk_surface, 
    Vulkan_Queues* wp_queues
) {
    _vk_physical_device = vk_physical_device;
    _vk_device = vk_device;
    _vk_surface = vk_surface;
    _wp_queues = wp_queues;
}

void Graphic::Vulkan_Command_Thread_Pool::init_item(Vulkan_Command_Thread_Item* command_thread_item) {
    command_thread_item->init(
        _vk_physical_device,
        _vk_device,
        _vk_surface,
        _wp_queues  
    );
}

void Graphic::Vulkan_Command_Thread_Pool::destroy_item(Vulkan_Command_Thread_Item* command_thread_item) {
    command_thread_item->destroy();
}

void Graphic::Vulkan_Command_Pool::init(
    VkPhysicalDevice vk_physical_device, 
    VkDevice vk_device, 
    VkSurfaceKHR vk_surface,
    Vulkan_Queues* wp_queues
) {
    _vk_commands_thread_pool = new Vulkan_Command_Thread_Pool(
        vk_physical_device,
        vk_device,
        vk_surface,
        wp_queues
    );

    _vk_commands_thread_pool->start_running(10);
}

/**
 * Vulkan_Command_Pool field
 */

void Graphic::Vulkan_Command_Pool::_record_single_commands(
    Vulkan_Commands_Mode commands_mode,
    const Vulkan_Commands_Record_Data& record_data
) {
    Utility::Log::get()->log_info("record_single_command 1", "commands_mode", commands_mode);
    Vulkan_Command_Task_Info task_info {
        record_data.record,
        commands_mode,
        record_data.callback
    };
    switch (commands_mode) {
        case Vulkan_Commands_Mode::COMMANDS_MODE_SYNC: {
            Utility::Log::get()->log_info("record_single_command 2");
            long task_id = _vk_commands_thread_pool->push_task(task_info);
            Utility::Log::get()->log_info("record_single_command 2.1", "task_id: ", task_id);
            _vk_commands_thread_pool->wait_to_task_end(task_id);
            Utility::Log::get()->log_info("record_single_command 2.2", "task_id: ", task_id);
            Utility::Log::get()->log_info("record_single_command 3", "task_id: ", task_id);
            break;
        }
        case Vulkan_Commands_Mode::COMMANDS_MODE_ASYNC: {
            Utility::Log::get()->log_info("record_single_command 4", "record", (void*)(&task_info.record));
            _vk_commands_thread_pool->push_task(task_info);
            break;
        }
    }
}

void Graphic::Vulkan_Command_Pool::record_single_commands(
    Vulkan_Commands_Mode commands_mode,
    Vulkan_Command_Record record,
    Vulkan_Command_Callback callback
) {
    _record_single_commands(
        commands_mode,
        Vulkan_Commands_Record_Data {
            record,
            callback
        }
    );
}

void Graphic::Vulkan_Command_Pool::record_sequence_commands(
    Vulkan_Commands_Mode commands_mode,
    std::vector<Vulkan_Commands_Record_Data>& records
) {
    for (int i = records.size() - 1;i > 0;i--) {
        auto record = records[i];
        auto& prev_record = records[i - 1];
        Vulkan_Command_Callback callback = prev_record.callback;
        Vulkan_Command_Callback wapper_callback = [this, commands_mode, record, callback, i] () {
            if (callback != nullptr) {
                callback();
            }
            Utility::Log::get()->log_info("record_sequence_commands 2", "record", i, commands_mode);
            this->_record_single_commands(
                commands_mode,
                record
            );
        };
        prev_record.callback = wapper_callback;
    }
    _record_single_commands(
        commands_mode,
        records[0]
    );
}

void Graphic::Vulkan_Command_Pool::destroy() {

    _vk_commands_thread_pool->destroy();

    delete(_vk_commands_thread_pool);
}