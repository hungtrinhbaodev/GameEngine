#include <vulkan/vk_core.h>
#include <vulkan/vk_buffer.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

    Buffer::Buffer() {
        
    }

    Buffer::Buffer(const Buffer& other) {
        size = other.size;
        usage_flags = other.usage_flags;
        property_flags = other.property_flags;
        device = other.device;
        buffer = other.buffer;
        memory = other.memory;
    };

    void Buffer::make_buffer(
        uint32_t size,
        VkBufferUsageFlags usage_flags,
        VkMemoryPropertyFlags property_flags,
        VkPhysicalDevice physical_device,
        VkDevice device
    ) {

        if (device == VK_NULL_HANDLE) {
            device = Vulkan::device;
        }

        if (physical_device == VK_NULL_HANDLE) {
            physical_device = Vulkan::physical_device;
        }

        this->usage_flags = usage_flags;
        this->property_flags = property_flags;
        this->size = size;
        this->device = device;

        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.usage = usage_flags;
        buffer_info.size = static_cast<VkDeviceSize>(size);
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        Utils::vk_check_result(
            vkCreateBuffer(device, &buffer_info, nullptr, &buffer),
            "",
            "Vulkan fail to create buffer!"
        );

        VkMemoryRequirements memory_requirement;
        vkGetBufferMemoryRequirements(device, buffer, &memory_requirement);

        VkMemoryAllocateInfo allocate_info{};
        allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate_info.allocationSize = static_cast<VkDeviceSize>(size);
        allocate_info.memoryTypeIndex = Utils::find_suitable_memory_type(
            memory_requirement.memoryTypeBits, 
            property_flags,
            physical_device
        );

        Utils::vk_check_result(
            vkAllocateMemory(device, &allocate_info, nullptr, &memory),
            "",
            "Vulkan fail to allocate buffer's memory!"
        );

        vkBindBufferMemory(device, buffer, memory, 0);
    }

    void Buffer::copy_data(uint32_t size, void* data) {
        if (property_flags != (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
            throw std::runtime_error("Vulkan fail to copy data to buffer: wrong type buffer to direct copy!");
        }
        void* map_memory = nullptr;
        vkMapMemory(device, memory, 0, size, 0, &map_memory);
            memcpy(map_memory, data, static_cast<size_t>(size));
        vkUnmapMemory(device, memory);
    }


    void Buffer::destroy(VkDevice device) {

        if (device == VK_NULL_HANDLE) {
            device = Vulkan::device;
        }

        vkDestroyBuffer(device, buffer, nullptr);

        vkFreeMemory(device, memory, nullptr);
    }

}