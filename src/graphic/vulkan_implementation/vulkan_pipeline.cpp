#include <graphic/vulkan_implementation/vulkan_pipeline.h>

/**
 * Pipeline field
 */

 VkShaderModule Graphic::Vulkan_Pipeline::_create_shader_module(
    VkDevice vk_device,
    const std::vector<char>& code
) {
    VkShaderModule shader_module{};

    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    Vulkan_Utility::vk_check_action(
        vkCreateShaderModule(vk_device, &create_info, nullptr, &shader_module),
        "Fail to create module shader!"
    );

    Utility::Log::get()->log_info("Create shader module success!");

    return shader_module;
}

VkPipelineShaderStageCreateInfo Graphic::Vulkan_Pipeline::_create_stage_pipeline(
    const VkShaderStageFlagBits& stage, 
    VkShaderModule& module, 
    const char* entry
) {
    VkPipelineShaderStageCreateInfo create_stage_info{};

    create_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    create_stage_info.stage = stage;
    create_stage_info.module = module;
    create_stage_info.pName = entry;

    return create_stage_info;
}

VkPipelineVertexInputStateCreateInfo Graphic::Vulkan_Pipeline::_create_vertex_input_info(
    const Vulkan_Pipeline_Config& pipeline_config
) {
    VkPipelineVertexInputStateCreateInfo info{};

    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    info.vertexBindingDescriptionCount = static_cast<uint32_t>(pipeline_config.vertex_descriptions.size());
    info.pVertexBindingDescriptions = pipeline_config.vertex_descriptions.data();

    info.vertexAttributeDescriptionCount = static_cast<uint32_t>(pipeline_config.vertex_attributes.size());
    info.pVertexAttributeDescriptions = pipeline_config.vertex_attributes.data();

    return info;
}

VkPipelineInputAssemblyStateCreateInfo Graphic::Vulkan_Pipeline::_create_input_assembly_info() {
    VkPipelineInputAssemblyStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    info.primitiveRestartEnable = VK_FALSE;
    return info;
}

VkPipelineViewportStateCreateInfo Graphic::Vulkan_Pipeline::_create_viewport_info() {
    VkPipelineViewportStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    info.viewportCount = 1;
    info.scissorCount = 1;
    return info;
}

VkPipelineRasterizationStateCreateInfo Graphic::Vulkan_Pipeline::_create_rasterize_info() {
    VkPipelineRasterizationStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    info.depthClampEnable = VK_FALSE;
    info.rasterizerDiscardEnable = VK_FALSE;
    info.polygonMode = VK_POLYGON_MODE_FILL;
    info.lineWidth = 1.0f;
    info.cullMode = VK_CULL_MODE_BACK_BIT;
    info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    info.depthBiasEnable = VK_FALSE;
    return info;
}

VkPipelineMultisampleStateCreateInfo Graphic::Vulkan_Pipeline::_create_multi_sampling_info() {
    VkPipelineMultisampleStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    info.sampleShadingEnable = VK_FALSE;
    info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    return info;
}

VkPipelineColorBlendAttachmentState Graphic::Vulkan_Pipeline::_create_color_blend_attachment_info() {
    VkPipelineColorBlendAttachmentState info{};
    info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    info.blendEnable = VK_FALSE;
    return info;
}

VkPipelineColorBlendStateCreateInfo Graphic::Vulkan_Pipeline::_create_color_blending_info(VkPipelineColorBlendAttachmentState &attachment_color) {
    VkPipelineColorBlendStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    info.logicOpEnable = VK_FALSE;
    info.logicOp = VK_LOGIC_OP_COPY;
    info.attachmentCount = 1;
    info.pAttachments = &attachment_color;
    info.blendConstants[0] = 0.0f;
    info.blendConstants[1] = 0.0f;
    info.blendConstants[2] = 0.0f;
    info.blendConstants[3] = 0.0f;
    return info;
}

VkPipelineDynamicStateCreateInfo Graphic::Vulkan_Pipeline::_create_dynamic_state_info() {
    VkPipelineDynamicStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    info.dynamicStateCount = static_cast<uint32_t>(Vulkan_Constants::PIPELINE_DYNAMIC_STATES.size());
    info.pDynamicStates = Vulkan_Constants::PIPELINE_DYNAMIC_STATES.data();
    return info;
}

VkPipelineLayoutCreateInfo Graphic::Vulkan_Pipeline::_create_pipeline_layout_info(const Vulkan_Pipeline_Config& pipeline_config) {
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    const auto& vk_descriptor_set_layouts = pipeline_config.descriptor_set_layouts;
    info.setLayoutCount = vk_descriptor_set_layouts.size();
    info.pSetLayouts = vk_descriptor_set_layouts.data();
    return info;
}

void Graphic::Vulkan_Pipeline::init(
    VkDevice vk_device, 
    VkRenderPass vk_render_pass, 
    const Vulkan_Pipeline_Config& pipeline_config
) {
    std::vector<char> vert_code = Utility::File_Utils::get()->read_file_shader(pipeline_config.path_vert_shader);
    std::vector<char> frag_code = Utility::File_Utils::get()->read_file_shader(pipeline_config.path_frag_shader);

    Utility::Log::get()->log_info("Read file vert shader", pipeline_config.path_vert_shader, ":", vert_code.size());
    Utility::Log::get()->log_info("Read file frag shader", pipeline_config.path_frag_shader, ":", frag_code.size());

    VkShaderModule vertex_module = _create_shader_module(
        vk_device,
        vert_code
    );
    VkShaderModule fragment_module = _create_shader_module(
        vk_device,
        frag_code
    );

    VkPipelineShaderStageCreateInfo vert_shader_stage_info = _create_stage_pipeline(
        VK_SHADER_STAGE_VERTEX_BIT,
        vertex_module,
        "main"
    );
    VkPipelineShaderStageCreateInfo frag_shader_stage_info = _create_stage_pipeline(
        VK_SHADER_STAGE_FRAGMENT_BIT,
        fragment_module,
        "main"
    );

    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

    VkPipelineVertexInputStateCreateInfo vertex_input_info = _create_vertex_input_info(pipeline_config);

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info = _create_input_assembly_info();

    VkPipelineViewportStateCreateInfo view_port_info = _create_viewport_info();

    VkPipelineRasterizationStateCreateInfo rasterizer_info = _create_rasterize_info();

    VkPipelineMultisampleStateCreateInfo multi_sampling_info = _create_multi_sampling_info();

    VkPipelineColorBlendAttachmentState color_blend_attachment_info = _create_color_blend_attachment_info();

    VkPipelineColorBlendStateCreateInfo color_blend_info = _create_color_blending_info(color_blend_attachment_info);

    VkPipelineDynamicStateCreateInfo dynamic_state_info = _create_dynamic_state_info();

    VkPipelineLayoutCreateInfo layout_info = _create_pipeline_layout_info(pipeline_config);

    Vulkan_Utility::vk_check_action(
        vkCreatePipelineLayout(vk_device, &layout_info, nullptr, &_vk_pipeline_layout),
        "failed to create pipeline layout!"
    );
    Utility::Log::get()->log_info("Create pipeline layout success!");

    VkGraphicsPipelineCreateInfo pipeline_create_info{};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount = 2;
    pipeline_create_info.pStages = shader_stages;
    pipeline_create_info.pVertexInputState = &vertex_input_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_info;
    pipeline_create_info.pViewportState = &view_port_info;
    pipeline_create_info.pRasterizationState = &rasterizer_info;
    pipeline_create_info.pMultisampleState = &multi_sampling_info;
    pipeline_create_info.pColorBlendState = &color_blend_info;
    pipeline_create_info.pDynamicState = &dynamic_state_info;
    pipeline_create_info.layout = _vk_pipeline_layout;
    pipeline_create_info.renderPass = vk_render_pass;
    pipeline_create_info.subpass = 0;
    pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;

    Vulkan_Utility::vk_check_action(
        vkCreateGraphicsPipelines(vk_device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &_vk_pipeline),
        "failed to create graphics pipeline!"
    );
    Utility::Log::get()->log_info("Create graphic pipeline success!");

    // remove module shader after create pipeline finish
    vkDestroyShaderModule(vk_device, vertex_module, nullptr);
    vkDestroyShaderModule(vk_device, fragment_module, nullptr);
 }

VkPipeline Graphic::Vulkan_Pipeline::get() {
    return _vk_pipeline;
}

VkPipelineLayout Graphic::Vulkan_Pipeline::get_layout() {
    return _vk_pipeline_layout;
}

void Graphic::Vulkan_Pipeline::destroy(VkDevice vk_device) {

    // destroy vulkan pipeline 
    vkDestroyPipeline(vk_device, _vk_pipeline, nullptr);
    Utility::Log::get()->log_info("Destroy pipeline success!");

    // destroy vulkan pipeline layout
    vkDestroyPipelineLayout(vk_device, _vk_pipeline_layout, nullptr);
    Utility::Log::get()->log_info("Destroy pipeline layout success!");
}

/**
 * Builder pipeline field
 */

Graphic::Vulkan_Pipeline_Builder& Graphic::Vulkan_Pipeline_Builder::add_pipeline_config(const Vulkan_Pipeline_Config& pipeline_config) {
    _pipeline_config = pipeline_config;
    return *this;
}

Graphic::Vulkan_Pipeline* Graphic::Vulkan_Pipeline_Builder::build() {
    _instance = new Vulkan_Pipeline();
    _instance->init(
        _vk_device,
        _vk_render_pass,
        _pipeline_config
    );
    return _instance;
}