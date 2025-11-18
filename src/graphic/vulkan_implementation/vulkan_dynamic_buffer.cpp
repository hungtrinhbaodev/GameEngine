#include <graphic/vulkan_implementation/vulkan_dynamic_buffer.h>

void Graphic::Vulkan_Dynamic_Buffer::make(
    VkBufferUsageFlags usage, 
    VkMemoryPropertyFlags property_flags,
    bool is_auto_map_memory
) {
    // save info of buffer to using in dynamic resize phase
    const auto& vk_default_device = Vulkan_Utility::get_or_default_device(VK_NULL_HANDLE, VK_NULL_HANDLE);
    _vk_device = vk_default_device.vk_device;
    _usage = usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    _property_flags = property_flags;
    _is_auto_map_memory = is_auto_map_memory;
}

uint32_t Graphic::Vulkan_Dynamic_Buffer::_on_resize(uint32_t additional_size) {
    if (additional_size <= 0) return additional_size;

    // if current buffer does not have data
    // we just make it
    Utility::Log::get()->log_info("_on_resize 1", _vk_buffer, additional_size, _size);
    if (_size <= 0) {
        Utility::Log::get()->log_info("_on_resize 2", _vk_buffer);
        Vulkan_Buffer::make(
            additional_size,
            _usage,
            _property_flags,
            _is_auto_map_memory
        );
        Utility::Log::get()->log_info("_on_resize 3", _vk_buffer);
        return additional_size;
    }
    // else we reserve 1.2 size buffer
    // after addition for future using
    else {

        uint32_t additional_size_need = uint32_t((_size + additional_size) * 0.5f);

        // make staging buffer
        Vulkan_Buffer* staging_buffer = new Vulkan_Buffer();

        staging_buffer->make(
            _size + additional_size + additional_size_need,
            _usage,
            _property_flags
        );

        Utility::Log::get()->log_info("_on_resize 4", staging_buffer->get_vk_device_memory(), _vk_device_memory);
        // make a copy to template buffer
        Vulkan_Buffer::copy_buffer(
            Vulkan_Commands_Mode::COMMANDS_MODE_SYNC,
            this,
            staging_buffer,
            {{0, 0, _size}},
            [this, staging_buffer] () {

                // swap two buffer with each other
                swap_with_other(*staging_buffer);

                // destroy the staging is swapped
                staging_buffer->destroy();
            }
        );

        Utility::Log::get()->log_info("_on_resize 5", _size, additional_size_need);

        return additional_size + additional_size_need;
    }
}

void Graphic::Vulkan_Dynamic_Buffer::_copy_data_to_offset(
    void* src_data,
    uint32_t src_offset,
    uint32_t dst_offset,
    uint32_t size
) {
    if (_property_flags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        void* src_cpy_data = (uint8_t*)src_data + src_offset;
        if (_is_auto_map_memory) {
            void* dst_cpy_data = (uint8_t*)_map_ptr + dst_offset;
            memcpy(dst_cpy_data, src_cpy_data, size);
        }
        else {
            map_memory();
            void* dst_cpy_data = (uint8_t*)_map_ptr + dst_offset;
            memcpy(dst_cpy_data, src_cpy_data, size);
            unmap_memory();
        }
    }
    else {
        // we make a staging buffer and copy that
        Vulkan_Buffer* staging_buffer = new Vulkan_Buffer();
        void* src_cpy_data = (uint8_t*)src_data + src_offset;
        staging_buffer->make(
            size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        staging_buffer->map_and_copy_data(src_cpy_data, size);
        Vulkan_Buffer::copy_buffer(
            Vulkan_Commands_Mode::COMMANDS_MODE_SYNC,
            staging_buffer,
            this,
            {{0, dst_offset, size}},
            [staging_buffer] () {
                staging_buffer->destroy();
            }
        );
    }

}

void Graphic::Vulkan_Dynamic_Buffer::log_buffer_data(const std::string& prefix) {
    // in case _property_flags is VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT we can't map it
    if (_property_flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) return;
    if (!_is_auto_map_memory) {
        map_memory();
        const auto& parse_data = Utility::Func_Utils::parse_data<int>(_map_ptr, 0, _size);
        Utility::Log::get()->log_info("append data", prefix, parse_data, "buffer propertices flag:", _property_flags);
        unmap_memory();
    }
    else {
        const auto& parse_data = Utility::Func_Utils::parse_data<int>(_map_ptr, 0, _size);
        Utility::Log::get()->log_info("append data", prefix, parse_data, "buffer propertices flag:", _property_flags);
    }
}

VkBuffer Graphic::Vulkan_Dynamic_Buffer::request_using_buffer() {
    _buffer_lock.lock();
    return Vulkan_Buffer::get();
}

void Graphic::Vulkan_Dynamic_Buffer::release_using_buffer() {
    _buffer_lock.unlock();
}

Graphic::Vulkan_Dynamic_Buffer::~Vulkan_Dynamic_Buffer() {

}