#pragma once
#include <iostream>
#include <map>

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
        
        public:

        void make(VkBufferUsageFlags usage, VkMemoryPropertyFlags property_flags);
    };
 
}