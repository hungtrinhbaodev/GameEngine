#pragma once
#include <vulkan/vulkan.h>

#include <functional>

#include <core/concurent_pool.hpp>
#include <core/thread_pool.hpp>
#include <graphic/vulkan_implementation/vulkan_constants.h>
#include <graphic/vulkan_implementation/vulkan_queues.h>
#include <graphic/vulkan_implementation/vulkan_queue_family_indices.h>


namespace Graphic {

    using Vulkan_Command_Record = std::function<void(VkCommandBuffer)>;

    using Vulkan_Command_Callback = std::function<void()>;

    struct Vulkan_Command_Task_Info {

        Vulkan_Command_Record record;

        Vulkan_Commands_Mode commands_mode;

        Vulkan_Command_Callback callback;
    };
    
    class Vulkan_Command_Buffer_Pool : public Core::Concurent_Pool<VkCommandBuffer> {

        private:

        VkDevice _vk_device;

        VkCommandPool _vk_command_pool;

        VkCommandBuffer _create_item();

        void _delete_item(VkCommandBuffer& item);

        public:

        void init(
            VkDevice vk_device,
            VkCommandPool vk_command_pool
        );

        void pooling_item(const VkCommandBuffer& item);

    };

    class Vulkan_Command_Thread_Item : public Core::Thread_Item<Vulkan_Command_Task_Info> {

        private:

        VkCommandPool _vk_command_pool;

        VkDevice _vk_device;

        Vulkan_Queues* _wp_queues;

        Vulkan_Command_Buffer_Pool _wp_command_buffer_pool;

        public:

        void init(
            VkPhysicalDevice vk_physical_device,
            VkDevice vk_device, 
            VkSurfaceKHR vk_surface, 
            Vulkan_Queues* wp_queues
        );

        void destroy();

        void do_task(Vulkan_Command_Task_Info task);

        VkCommandBuffer request_command_buffer();

        ~Vulkan_Command_Thread_Item();

    };

    class Vulkan_Command_Thread_Pool : public Core::Thread_Pool<Vulkan_Command_Thread_Item, Vulkan_Command_Task_Info> {
        
        private:

        VkPhysicalDevice _vk_physical_device;

        VkDevice _vk_device;

        VkSurfaceKHR _vk_surface;

        Vulkan_Queues* _wp_queues;

        public:

        Vulkan_Command_Thread_Pool(
            VkPhysicalDevice vk_physical_device,
            VkDevice vk_device, 
            VkSurfaceKHR vk_surface, 
            Vulkan_Queues* queues
        );

        void init_item(Vulkan_Command_Thread_Item* command_thread_item);

        void destroy_item(Vulkan_Command_Thread_Item* command_thread_item);

    };

    struct Vulkan_Commands_Record_Data {
        Vulkan_Command_Record record;
        Vulkan_Command_Callback callback = nullptr;
    };

    class Vulkan_Command_Pool {

        private:

        Vulkan_Command_Thread_Pool* _vk_commands_thread_pool;

        Vulkan_Command_Thread_Item _vk_command_main_thread_item;

        void _record_single_commands(
            Vulkan_Commands_Mode commands_mode,
            const Vulkan_Commands_Record_Data& record_data
        );

        public:

        void init(
            VkPhysicalDevice vk_physical_device, 
            VkDevice vk_device,
            VkSurfaceKHR vk_surface,
            Vulkan_Queues* wp_queues
        );

        void record_single_commands(
            Vulkan_Commands_Mode commands_mode,
            Vulkan_Command_Record record,
            Vulkan_Command_Callback callback = nullptr
        );

        void record_sequence_commands(
            Vulkan_Commands_Mode commands_mode,
            std::vector<Vulkan_Commands_Record_Data>& records
        );

        /**
         * This buffer will be only used
         * in main thread graphic
         */
        VkCommandBuffer request_draw_command_buffer();

        void destroy();
    };

}