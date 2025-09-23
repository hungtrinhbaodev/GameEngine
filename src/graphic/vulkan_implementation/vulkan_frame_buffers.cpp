#include <graphic/vulkan_implementation/vulkan_frame_buffers.h>

void Graphic::Vulkan_Frame_Buffers::init(
    VkDevice vk_device,
    VkRenderPass vk_render_pass,
    const std::vector<VkImageView>& vk_swapchain_imageviews,
    VkExtent2D vk_swapchain_extent
) {
    _vk_frame_buffers.resize(vk_swapchain_imageviews.size());

    for(size_t i = 0;i < vk_swapchain_imageviews.size();i++){
        VkImageView attachments[] = {
            vk_swapchain_imageviews[i]
        };

        VkFramebufferCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = vk_render_pass;
        create_info.attachmentCount = 1;
        create_info.pAttachments = attachments;
        create_info.width = vk_swapchain_extent.width;
        create_info.height = vk_swapchain_extent.height;
        create_info.layers = 1;

        Vulkan_Utility::vk_check_action(
            vkCreateFramebuffer(vk_device, &create_info, nullptr, &_vk_frame_buffers[i]),
            "failed to create framebuffer!"
        );
    }
    Utility::Log::get()->log_info("Create frame buffers success!");
}

const std::vector<VkFramebuffer>& Graphic::Vulkan_Frame_Buffers::get() {
    return _vk_frame_buffers;
}

void Graphic::Vulkan_Frame_Buffers::destroy(VkDevice vk_device) {
    for(auto& frame_buffer : _vk_frame_buffers){
        vkDestroyFramebuffer(vk_device, frame_buffer, nullptr);
    }
    Utility::Log::get()->log_info("Destroy swap chain frame buffers success!");
}