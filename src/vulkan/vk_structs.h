#pragma once
#include <vulkan/vulkan.h>

namespace Vulkan {

    namespace Structs {

        inline VkCommandBufferBeginInfo make_command_begin_info(VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) {
            VkCommandBufferBeginInfo begin_info{};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags = flags;
            return begin_info;
        }

        inline VkSubmitInfo make_submit_info(VkCommandBuffer* command_buffer, uint32_t command_count = 1) {
            VkSubmitInfo submit_info{};
            submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = command_count;
            submit_info.pCommandBuffers = command_buffer;
            return submit_info;
        }

    }

}