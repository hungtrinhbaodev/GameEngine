#include <graphic/vulkan_implementation/vulkan_render_pass.h>

void Graphic::Vulkan_Render_Pass::init(
    VkDevice vk_device, 
    VkPhysicalDevice vk_physical_device,
    VkFormat vk_swapchain_format
) {

    // color attachment info
    VkAttachmentDescription color_attachment{};
    color_attachment.format = vk_swapchain_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth_attachment{};
    depth_attachment.format = Vulkan_Utility::find_depth_format(vk_physical_device);
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // color attachment ref
    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref{};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // sub pass description
    VkSubpassDescription sub_pass{};
    sub_pass.colorAttachmentCount = 1;
    sub_pass.pColorAttachments = &color_attachment_ref;
    sub_pass.pDepthStencilAttachment = &depth_attachment_ref;
    // sub_pass.inputAttachmentCount = 0;
    // sub_pass.pInputAttachments = nullptr;
    // sub_pass.pResolveAttachments = nullptr; 

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // render pass create info
    std::vector<VkAttachmentDescription> attachments = {color_attachment, depth_attachment};
    VkRenderPassCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount = attachments.size();
    create_info.pAttachments = attachments.data();
    create_info.subpassCount = 1;
    create_info.pSubpasses = &sub_pass;
    create_info.dependencyCount = 1;
    create_info.pDependencies = &dependency;

    // create render pass
    Vulkan_Utility::vk_check_action(
        vkCreateRenderPass(vk_device, &create_info, nullptr, &_vk_render_pass),
        "fail to create render pass!"
    );
    Utility::Log::get()->log_info("Create render pass success!");
}

VkRenderPass Graphic::Vulkan_Render_Pass::get() {
    return _vk_render_pass;
}

void Graphic::Vulkan_Render_Pass::destroy(VkDevice vk_device) {
    vkDestroyRenderPass(vk_device, _vk_render_pass, nullptr);
    Utility::Log::get()->log_info("Destroy render pass success!");
}