#include <graphic/vulkan_implementation/vulkan_buffer.h>
#include <graphic/vulkan_implementation/vulkan_core_data.h>

void Graphic::Vulkan_Buffer::make(
    uint32_t size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags property_flags,
    bool is_auto_map_memory
) {

    const auto& vk_default_data = Vulkan_Utility::get_or_default_device(VK_NULL_HANDLE, VK_NULL_HANDLE);
    VkPhysicalDevice vk_physical_device = vk_default_data.vk_physical_device;
    VkDevice vk_device = vk_default_data.vk_device;
    Utility::Log::get()->log_info("Graphic::Vulkan_Buffer::make 1", vk_device);

    _vk_device = vk_device;
    _usage = usage;
    _property_flags = property_flags;
    _size = size;
    _is_auto_map_memory = is_auto_map_memory;

    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.usage = usage;
    buffer_info.size = static_cast<VkDeviceSize>(size);
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    Vulkan_Utility::vk_check_action(
        vkCreateBuffer(_vk_device, &buffer_info, nullptr, &_vk_buffer),
        "fail to create buffer!"
    );

    VkMemoryRequirements memory_requirement;
    vkGetBufferMemoryRequirements(_vk_device, _vk_buffer, &memory_requirement);

    Utility::Log::get()->log_info("VkMemoryRequirements buffer", memory_requirement.size, memory_requirement.alignment, memory_requirement.memoryTypeBits);

    VkMemoryAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = static_cast<VkDeviceSize>(size);
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

    if(_is_auto_map_memory) {
        map_memory();
    }
}

void Graphic::Vulkan_Buffer::map_memory() {
    switch (_map_state) {
        case Vulkan_Buffer_Map_Memory_State::UNMAP: {
            vkMapMemory(
                _vk_device, 
                _vk_device_memory, 
                0, 
                static_cast<VkDeviceSize>(_size), 
                0,
                &_map_ptr
            );
            _map_state = Vulkan_Buffer_Map_Memory_State::MAPPED;
            break;
        }
        default:{
            break;
        }
    }
    
}

void Graphic::Vulkan_Buffer::unmap_memory() {
    switch (_map_state) {
        case Vulkan_Buffer_Map_Memory_State::MAPPED: {
            vkUnmapMemory(_vk_device, _vk_device_memory);
            _map_state = Vulkan_Buffer_Map_Memory_State::UNMAP;
            break;
        }
        default: {
            break;
        }
    }
}

void Graphic::Vulkan_Buffer::copy_data(void * data_src, uint32_t size) {
    memcpy(this->_map_ptr, data_src, static_cast<size_t>(size));
}

void Graphic::Vulkan_Buffer::map_and_copy_data(void * data_src, uint32_t size) {
    if ((VkDeviceSize) size != this->_size) {
        throw std::runtime_error("fail to copy data to buffer!");
    }
    map_memory();
        copy_data(data_src, size);
    unmap_memory();
}

void Graphic::Vulkan_Buffer::swap_with_other(Vulkan_Buffer& other) {
    unmap_memory();
    other.unmap_memory();

    Utility::Func_Utils::swap(_vk_buffer, other._vk_buffer);
    Utility::Func_Utils::swap(_size, other._size);
    Utility::Func_Utils::swap(_map_ptr, other._map_ptr);
    Utility::Func_Utils::swap(_vk_device_memory, other._vk_device_memory);
    Utility::Func_Utils::swap(_usage, other._usage);
    Utility::Func_Utils::swap(_property_flags, other._property_flags);

    if (_is_auto_map_memory) {
        map_memory();
    }

    if (other._is_auto_map_memory) {
        other.map_memory();
    }
}

VkBuffer Graphic::Vulkan_Buffer::get() {
    return _vk_buffer;
}

VkDeviceSize Graphic::Vulkan_Buffer::get_size() {
    return _size;
}

VkDeviceMemory& Graphic::Vulkan_Buffer::get_vk_device_memory() {
    return _vk_device_memory;
}

void Graphic::Vulkan_Buffer::destroy() {
    vkDestroyBuffer(_vk_device, _vk_buffer, nullptr);

    vkFreeMemory(_vk_device, _vk_device_memory, nullptr);

    Utility::Log::get()->log_info("Graphic::Vulkan_Buffer::destroy", _vk_buffer, _vk_device);
}

void Graphic::Vulkan_Buffer::copy_buffer(
    Vulkan_Commands_Mode commands_mode,
    Vulkan_Buffer* src,
    Vulkan_Buffer* dst,
    const std::vector<VkBufferCopy>& copy_regions,
    Vulkan_Command_Callback callback
) {

    const auto& vk_default_wp = Vulkan_Utility::get_or_default_submit(nullptr, nullptr);

    vk_default_wp.command_pool->record_single_commands(
        commands_mode,
        [copy_regions, src, dst](VkCommandBuffer vk_command_buffer) {
            vkCmdCopyBuffer(vk_command_buffer, src->get(), dst->get(), copy_regions.size(), copy_regions.data());
        },
        callback
    );
}

void Graphic::Vulkan_Buffer::copy_buffer(
    Vulkan_Commands_Mode commands_mode,
    Vulkan_Buffer* src, 
    Vulkan_Buffer* dst,
    Vulkan_Command_Callback callback
) {

    if (src->get_size() != dst->get_size()) {
        throw std::runtime_error("fail to copy buffer to buffer!");
    }

    copy_buffer(
        commands_mode,
        src,
        dst,
        {{ 0, 0, dst->get_size()}},
        callback
    );
}