#include <graphic/vulkan_implementation/vulkan_buffer.h>
#include <graphic/vulkan_implementation/vulkan_core_data.h>

void Graphic::Vulkan_Buffer::make(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags property_flags,
    VkPhysicalDevice vk_physical_device,
    VkDevice vk_device
) {

    if (vk_physical_device == VK_NULL_HANDLE || vk_device == VK_NULL_HANDLE) {
        const auto& data = Vulkan_Core_Data::get()->get_raw_data();
        vk_device = data.device;
        vk_physical_device = data.physical_device;
    }

    if (vk_physical_device == VK_NULL_HANDLE || vk_device == VK_NULL_HANDLE) {
        Vulkan_Utility::vk_check_action(
            VK_INCOMPLETE,
            "fail to create vulkan buffer: not found device, physcal device!"
        );
        return;
    }

    _vk_device = vk_device;
    _size = size;

    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.usage = usage;
    buffer_info.size = size;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    Vulkan_Utility::vk_check_action(
        vkCreateBuffer(_vk_device, &buffer_info, nullptr, &_vk_buffer),
        "fail to create buffer!"
    );

    VkMemoryRequirements memory_requirement;
    vkGetBufferMemoryRequirements(_vk_device, _vk_buffer, &memory_requirement);

    VkMemoryAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = size;
    allocate_info.memoryTypeIndex = Vulkan_Utility::find_buffer_memory_type_index(
        memory_requirement.memoryTypeBits, 
        property_flags,
        vk_physical_device
    );

    Vulkan_Utility::vk_check_action(
        vkAllocateMemory(_vk_device, &allocate_info, nullptr, &_vk_device_memory),
        "fail to allocate buffer's memory!"
    );

    vkBindBufferMemory(_vk_device, _vk_buffer, _vk_device_memory, 0);
}

void Graphic::Vulkan_Buffer::map_memory() {
    vkMapMemory(
        _vk_device, 
        _vk_device_memory, 
        0, 
        _size, 
        0, 
        &_map_ptr
    );
}

void Graphic::Vulkan_Buffer::unmap_memory() {
    vkUnmapMemory(_vk_device, _vk_device_memory);
}

void Graphic::Vulkan_Buffer::copy(void * data_src, size_t size) {
    memcpy(this->_map_ptr, data_src, size);
}

void Graphic::Vulkan_Buffer::map_and_copy(void * data_src, size_t size) {
    if ((VkDeviceSize) size != this->_size) {
        throw std::runtime_error("fail to copy data to buffer!");
    }
    map_memory();
        copy(data_src, size);
    unmap_memory();
}

VkBuffer Graphic::Vulkan_Buffer::get() {
    return _vk_buffer;
}

VkDeviceSize Graphic::Vulkan_Buffer::get_size() {
    return _size;
}

void Graphic::Vulkan_Buffer::destroy() {
    
    vkDestroyBuffer(_vk_device, _vk_buffer, nullptr);

    vkFreeMemory(_vk_device, _vk_device_memory, nullptr);
}