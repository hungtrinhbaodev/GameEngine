#include <future>

#include <vulkan/vk_core.h>
#include <vulkan/vk_image.h>
#include <vulkan/vk_command_pool.h>
#include <vulkan/vk_utils.h>
#include <vulkan/vk_structs.h>
#include <vulkan/vk_queues.h>
#include <vulkan/vk_fences.h>
#include <vulkan/vk_buffer.h>

namespace Vulkan {

    Image::Image() {

    }

    Image::Image(const Image& other) {
        width = other.width;
        height = other.height;
        view = other.view;
        format = other.format;
        sampler = other.sampler;
        aspect_flags = other.aspect_flags;
        descriptor = other.descriptor;
    }

    void Image::make_image(
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImageAspectFlags aspect_flags,
        VkPhysicalDevice physical_device,
        VkDevice device
    ) {

        if (device == VK_NULL_HANDLE) {
            device = Vulkan::device;
        }

        if (physical_device == VK_NULL_HANDLE) {
            physical_device = Vulkan::physical_device;
        }

        this->device = device;
        this->aspect_flags = aspect_flags;
        this->format = format;
        this->width = width;
        this->height = height;

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

        if(vkCreateImage(device, &create_info, nullptr, &image) != VK_SUCCESS){
            throw std::runtime_error("Fail to create image!");
        }

        VkMemoryRequirements memory_requirements{};
        vkGetImageMemoryRequirements(device, image, &memory_requirements);

        VkMemoryAllocateInfo allocate_info{};
        allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate_info.allocationSize = memory_requirements.size;
        allocate_info.memoryTypeIndex = Utils::find_suitable_memory_type(
            memory_requirements.memoryTypeBits, 
            properties,
            physical_device
        );

        Utils::vk_check_result(
            vkAllocateMemory(device, &allocate_info, nullptr, &memory),
            "",
            "Vulkan fail to allocate image memory!"
        );

        vkBindImageMemory(device, image, memory, 0);

        view = Utils::create_imageview_from_image(
            image,
            format,
            aspect_flags,
            device
        );

    }

    void Image::transition_image_layout(
        VkImageLayout old_layout,
        VkImageLayout new_layout
    ) {
        auto result = _global_thread_pool->enqueue([this] (VkImageLayout old_layout, VkImageLayout new_layout) {

            auto thread_id = std::this_thread::get_id();
            VkCommandBuffer command_buffer = API::request_command_buffer();
            VkCommandBufferBeginInfo begin_info = Structs::make_command_begin_info();

            vkBeginCommandBuffer(command_buffer, &begin_info);

                VkImageMemoryBarrier barrier_info{};
                barrier_info.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier_info.oldLayout = old_layout;
                barrier_info.newLayout = new_layout;
                barrier_info.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier_info.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier_info.image = image;
                barrier_info.subresourceRange.aspectMask = aspect_flags;
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
                else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
                    barrier_info.srcAccessMask = 0;
                    barrier_info.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

                    src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                } 
                else {
                    throw std::runtime_error("Vulkan transfer layout are not supported!");
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

            vkEndCommandBuffer(command_buffer);

            VkSubmitInfo submit_info = Structs::make_submit_info(&command_buffer);
            VkFence fence = API::request_fence();
            API::submit(
                submit_info,
                fence
            );

            return API::on_fence_success(fence, [] (VkFence fence, VkCommandBuffer command_buffer, std::thread::id thread_id) {
                API::release_command_buffer(command_buffer, thread_id);
                API::release_fence(fence);
            }, fence, command_buffer, thread_id);
            
        }, old_layout, new_layout);

        result.get().get();
    } 

    void Image::copy_image_data(uint32_t width, uint32_t height, void* pixels) {

        if (this->width != width || this->height != height) {
            throw std::runtime_error("Vulkan fail to copy image data: wrong size image!");
        }

        auto result = _global_thread_pool->enqueue([this] (uint32_t width, uint32_t height, void* pixels) {

            VkDeviceSize image_size = width * height * 4;
            Buffer staging{};
            staging.make_buffer(
                image_size,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
            staging.copy_data(image_size, pixels);

            auto thread_id = std::this_thread::get_id();
            VkCommandBuffer command_buffer = API::request_command_buffer();
            VkCommandBufferBeginInfo begin_command = Structs::make_command_begin_info();

            vkBeginCommandBuffer(command_buffer, &begin_command);

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

                vkCmdCopyBufferToImage(
                    command_buffer, 
                    staging.buffer, 
                    image, 
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                    1,
                    &region
                );

            vkEndCommandBuffer(command_buffer);

            VkSubmitInfo submit_info = Structs::make_submit_info(&command_buffer);
            VkFence fence = API::request_fence();
            API::submit(submit_info, fence);

            return API::on_fence_success(fence, [] (VkFence fence, VkCommandBuffer command_buffer, Buffer buffer, std::thread::id thread_id) {

                buffer.destroy();
                API::release_fence(fence);
                API::release_command_buffer(command_buffer, thread_id);

            }, fence, command_buffer, std::move(staging), thread_id);

        }, width, height, pixels);

        result.get().get();
    }

    void Image::destroy() {

        if (view != VK_NULL_HANDLE) {
            vkDestroyImageView(device, view, nullptr);
        }

        if (memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, memory, nullptr);
        }

        if (image != VK_NULL_HANDLE) {
            vkDestroyImage(device, image, nullptr);
        }
    }

}