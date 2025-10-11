#include <graphic/vulkan_implementation/vulkan_image.h>

void Graphic::Vulkan_Image::make(
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties
) {
    const auto& vk_default_data = Vulkan_Utility::get_or_default_device(VK_NULL_HANDLE, VK_NULL_HANDLE);

    VkImageCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.imageType = VK_IMAGE_TYPE_2D;
    create_info.extent.width = width;
    create_info.extent.height = height;
    create_info.extent.depth = 1;
    create_info.arrayLayers = 1;
    create_info.format = format;
    create_info.tiling = tiling;
    create_info.mipLevels = 1;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    create_info.usage = usage;
    create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if(vkCreateImage(vk_default_data.vk_device, &create_info, nullptr, &_vk_image) != VK_SUCCESS){
        throw std::runtime_error("Fail to create image!");
    }

    VkMemoryRequirements memory_requirements{};
    vkGetImageMemoryRequirements(vk_default_data.vk_device, _vk_image, &memory_requirements);

    VkMemoryAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = Vulkan_Utility::find_buffer_memory_type_index(
        memory_requirements.memoryTypeBits, 
        properties
    );

    Vulkan_Utility::vk_check_action(
        vkAllocateMemory(vk_default_data.vk_device, &allocate_info, nullptr, &_vk_image_memory),
        "Fail to allocate image memory!"
    );

    vkBindImageMemory(vk_default_data.vk_device, _vk_image, _vk_image_memory, 0);

    _vk_imageview = Vulkan_Utility::create_imageview_from_image(
        _vk_image,
        format,
        vk_default_data.vk_device
    );
}

void Graphic::Vulkan_Image::transition_image_layout(
    Vulkan_Commands_Mode command_mode,
    VkFormat format,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    Vulkan_Command_Callback callback
) {
    const auto& vk_default_submit = Vulkan_Utility::get_or_default_submit(
        nullptr,
        nullptr
    );
    Utility::Log::get()->log_info("transition_image_layout 1");

    vk_default_submit.command_pool->record_single_commands(
        command_mode,
        [this, old_layout, new_layout] (VkCommandBuffer command_buffer) {
            Utility::Log::get()->log_info("transition_image_layout 2");

            VkImageMemoryBarrier barrier_info{};
            barrier_info.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier_info.oldLayout = old_layout;
            barrier_info.newLayout = new_layout;
            barrier_info.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier_info.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier_info.image = _vk_image;
            barrier_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier_info.subresourceRange.baseArrayLayer = 0;
            barrier_info.subresourceRange.baseMipLevel = 0;
            barrier_info.subresourceRange.levelCount = 1;
            barrier_info.subresourceRange.layerCount = 1;
            
            VkPipelineStageFlags src_stage;
            VkPipelineStageFlags dst_stage;

            if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                barrier_info.srcAccessMask = 0;
                barrier_info.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                barrier_info.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier_info.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else {
                throw std::runtime_error("Transfer layout are not supported!");
            }

            vkCmdPipelineBarrier(
                command_buffer,
                src_stage,
                dst_stage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier_info
            );
            // Utility::Log::get()->log_info("transition_image_layout 3");
        },
        callback
    );
}

void Graphic::Vulkan_Image::copy_buffer_to_image(
    Vulkan_Commands_Mode command_mode,
    int width,
    int height,
    const std::shared_ptr<Vulkan_Buffer> vk_staging_buffer,
    Vulkan_Command_Callback callback
) {
    // Utility::Log::get()->log_info("copy_buffer_to_image 1");
    const auto& vk_default_submit = Vulkan_Utility::get_or_default_submit(
        nullptr,
        nullptr
    );

    // Utility::Log::get()->log_info("copy_buffer_to_image 2");
    vk_default_submit.command_pool->record_single_commands(
        command_mode,
        [this, vk_staging_buffer, width, height] (VkCommandBuffer command_buffer) {
            // Utility::Log::get()->log_info("copy_buffer_to_image 3");

            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = {0, 0, 0};
            region.imageExtent = {
                (uint32_t) width,
                (uint32_t) height,
                1
            };
            // Utility::Log::get()->log_info("copy_buffer_to_image 4");

            vkCmdCopyBufferToImage(command_buffer, vk_staging_buffer->get(), _vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        },
        callback
    );

    // Utility::Log::get()->log_info("copy_buffer_to_image 5");
} 

VkImage Graphic::Vulkan_Image::get_image() {
    return _vk_image;
}

VkImageView Graphic::Vulkan_Image::get_imageview() {
    return _vk_imageview;
}

void Graphic::Vulkan_Image::destroy(VkDevice vk_device) {

    if (_vk_imageview != VK_NULL_HANDLE) {
        vkDestroyImageView(vk_device, _vk_imageview, nullptr);
    }

    if (_vk_image_memory != VK_NULL_HANDLE) {
        vkFreeMemory(vk_device, _vk_image_memory, nullptr);
    }

    if (_vk_image != VK_NULL_HANDLE) {
        vkDestroyImage(vk_device, _vk_image, nullptr);
    }
}