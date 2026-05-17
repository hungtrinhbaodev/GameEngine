#include <vulkan/vk_pipeline.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_structs.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

	void Pipeline::init(const Pipeline_Config& config) {

		VkDevice device = config.device;
		VkRenderPass render_pass = config.render_pass;

		if (device == VK_NULL_HANDLE) {
			device = Vulkan::device;
		}

		if (render_pass == VK_NULL_HANDLE) {
			render_pass = Vulkan::render_pass;
		}

		if (device == VK_NULL_HANDLE) {
			throw std::runtime_error("Vulkan fail to init pipeline: try to init device first!");
		}

		if (render_pass == VK_NULL_HANDLE) {
			throw std::runtime_error("Vulkan fail to init pipeline: try to init render pass first!");
		}

		// Make shader stage create info for vertex and fragment shader
		std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
		shader_stages.push_back(Structs::make_pipeline_shader_stage_create_info(
			config.vertex_shader_path,
			VK_SHADER_STAGE_VERTEX_BIT,
			device
		));
		shader_stages.push_back(Structs::make_pipeline_shader_stage_create_info(
			config.fragment_shader_path,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			device
		));

		// Make input assembly create info
		VkPipelineInputAssemblyStateCreateInfo input_assembly_info = Structs::make_pipeline_input_assembly_create_info(
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			VK_FALSE
		);

		// Make viewport and scissor info
		VkViewport viewport{};
		VkRect2D scissor{};
		VkPipelineViewportStateCreateInfo viewport_info = Structs::make_pipeline_view_port_create_info(
			config.swapchain_extent,
			viewport,
			scissor
		);

		// Make rasterization create info
		VkPipelineRasterizationStateCreateInfo rasterization_info = Structs::make_pipeline_rasterization_create_info();

		// Make multisampling create info
		VkPipelineMultisampleStateCreateInfo multi_sampling_info = Structs::make_pipeline_multisample_state_create_info();

		// Make depth test stencill create info
		VkPipelineDepthStencilStateCreateInfo depth_stencil_info = Structs::make_pipeline_depth_stencil_state_create_info();

		// Make color blend attachment info
		VkPipelineColorBlendAttachmentState color_attachment_state = Structs::make_pipeline_color_blend_attachment_state();
		VkPipelineColorBlendStateCreateInfo color_attachment_info = Structs::make_pipeline_color_blend_state_create_info(
			color_attachment_state
		);

		// Make dynamic state in pipeline crate info
		std::vector<VkDynamicState> dynamic_states{};
		VkPipelineDynamicStateCreateInfo dynamic_state_info = Structs::make_pipeline_dynamic_state_create_info(dynamic_states);

		// Make pipeline layout create info
		VkPushConstantRange push_constant_range = Structs::make_push_constant_range();
		VkPipelineLayoutCreateInfo pipeline_layout_info = Structs::make_pipeline_layout_create_info(
			config.descriptor_set_layouts,
			push_constant_range
		);
		
		// Create and check pipeline layout first
		Utils::vk_check_result(
			vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &layout),
			"Create pipeline layout successfully!",
			"Fail to create pipline layout"
		);

		VkGraphicsPipelineCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

		// pipeline stages vertex and fragment
		create_info.stageCount = shader_stages.size();
		create_info.pStages = shader_stages.data();

		// pipeline fix function state
		create_info.pVertexInputState = &config.vertex_input_create_info;
		create_info.pInputAssemblyState = &input_assembly_info;
		create_info.pTessellationState = nullptr;
		create_info.pViewportState = &viewport_info;
		create_info.pRasterizationState = &rasterization_info;
		create_info.pMultisampleState = &multi_sampling_info;
		create_info.pDepthStencilState = &depth_stencil_info;
		create_info.pColorBlendState = &color_attachment_info;
		create_info.pDynamicState = &dynamic_state_info;

		// pipeline layout and render
		create_info.layout = layout;
		create_info.renderPass = config.render_pass;
		create_info.subpass = 0;

		// pipeline derivatives — can speed up creating similar pipelines
		create_info.basePipelineHandle = VK_NULL_HANDLE;
		create_info.basePipelineIndex = -1;

		// Create pipeline with all above info
		Utils::vk_check_result(
			vkCreateGraphicsPipelines(
				device,
				nullptr,
				1,
				&create_info,
				nullptr,
				&pipeline
			),
			"Vulkan create pipeline successfully!",
			"Vulkan fail to create pipeline!"
		);

	}

	void Pipeline::destroy(VkDevice device) const {

		if (device == VK_NULL_HANDLE) {
			device = Vulkan::device;
		}

		if (device == VK_NULL_HANDLE) {
			throw std::runtime_error("Vulkan fail to destroy pipeline: try to init device first!");
		}

		vkDestroyPipelineLayout(device, layout, nullptr);
		Log::log_info("Vulkan destroy pipeline layout successfully!");

		vkDestroyPipeline(device, pipeline, nullptr);
		Log::log_info("Vulkan destroy pipeline successfully!");

	}

}