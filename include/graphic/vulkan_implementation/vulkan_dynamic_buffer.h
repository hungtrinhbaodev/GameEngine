#pragma once
#include <iostream>
#include <map>
#include <mutex>

#include <graphic/vulkan_implementation/vulkan_buffer.h>

namespace Graphic {

    class Vulkan_Dynamic_Buffer : public Vulkan_Buffer {

        protected:

        uint32_t _on_resize(uint32_t additional_size);

        void _copy_data_to_offset(
            void* src_data,
            uint32_t src_offset,
            uint32_t dst_offset,
            uint32_t size
        );

        std::mutex _buffer_lock;
        
        public:

        void log_buffer_data(const std::string& prefix = "");

        void make(
            VkBufferUsageFlags usage, 
            VkMemoryPropertyFlags property_flags,
            bool is_auto_map_memory = false
        );

        /**
         * request using buffer will return buffer
         * and make buffer in sync processing
         * and safe to using
         */
        VkBuffer request_using_buffer();

        void release_using_buffer();

        ~Vulkan_Dynamic_Buffer();
    };
 
}