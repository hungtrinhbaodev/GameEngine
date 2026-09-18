#pragma once
#include <vulkan/vulkan.h>
#include <files.h>

namespace Vulkan {

	namespace Structs {

		inline VkCommandBufferBeginInfo make_command_begin_info(
			VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		) {
			VkCommandBufferBeginInfo begin_info{};
			begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_info.flags = flags;
			return begin_info;
		}

		inline VkSubmitInfo make_submit_info(
			VkCommandBuffer* command_buffer, uint32_t command_count = 1, uint32_t wait_semaphore_count = 0,
			VkSemaphore* wait_semaphores = nullptr, VkPipelineStageFlags* wait_dst_stages = nullptr,
			uint32_t signal_semaphore_count = 0, VkSemaphore* signal_semaphores = nullptr
		) {
			VkSubmitInfo submit_info{};
			submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submit_info.commandBufferCount = command_count;
			submit_info.pCommandBuffers = command_buffer;
			submit_info.waitSemaphoreCount = wait_semaphore_count;
			submit_info.pWaitSemaphores = wait_semaphores;
			submit_info.pWaitDstStageMask = wait_dst_stages;
			submit_info.signalSemaphoreCount = signal_semaphore_count;
			submit_info.pSignalSemaphores = signal_semaphores;
			return submit_info;
		}

		inline VkPresentInfoKHR make_present_info(
			uint32_t wait_semaphore_count, VkSemaphore* wait_semaphores, uint32_t swapchain_count,
			VkSwapchainKHR* swapchains, uint32_t* image_index
		) {
			VkPresentInfoKHR present_info{};
			present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			present_info.waitSemaphoreCount = wait_semaphore_count;
			present_info.pWaitSemaphores = wait_semaphores;
			present_info.swapchainCount = swapchain_count;
			present_info.pSwapchains = swapchains;
			present_info.pImageIndices = image_index;
			return present_info;
		}

		inline VkShaderModule make_shader_module(const std::string& shader_path, VkDevice device) {
			std::vector<char> code = Files::read_file(shader_path);
			VkShaderModuleCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			create_info.codeSize = code.size();
			create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());
			VkShaderModule module{};
			vkCreateShaderModule(device, &create_info, nullptr, &module);
			return module;
		}

		inline VkPipelineShaderStageCreateInfo make_pipeline_shader_stage_create_info(
			VkShaderModule module, VkShaderStageFlagBits stage
		) {
			VkPipelineShaderStageCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			create_info.stage = stage;
			create_info.module = module;
			create_info.pName = "main";
			return create_info;
		}

		inline VkPipelineVertexInputStateCreateInfo make_pipeline_vertex_input_state_create_info(
			const std::vector<VkVertexInputBindingDescription>& vertex_binding_descriptions,
			const std::vector<VkVertexInputAttributeDescription>& vertex_attribute_descriptions
		) {
			VkPipelineVertexInputStateCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

			create_info.vertexBindingDescriptionCount = vertex_binding_descriptions.size();
			create_info.pVertexBindingDescriptions = vertex_binding_descriptions.data();

			create_info.vertexAttributeDescriptionCount = vertex_attribute_descriptions.size();
			create_info.pVertexAttributeDescriptions = vertex_attribute_descriptions.data();

			return create_info;
		}

		inline std::vector<VkDescriptorSet> make_descriptor_set(
			VkDescriptorPool descriptor_pool, uint32_t descriptor_set_count, VkDescriptorSetLayout* set_layout,
			VkDevice device
		) {
			std::vector<VkDescriptorSet> descriptor_sets(descriptor_set_count);
			VkDescriptorSetAllocateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			create_info.descriptorPool = descriptor_pool;
			create_info.descriptorSetCount = descriptor_set_count;
			create_info.pSetLayouts = set_layout;
			vkAllocateDescriptorSets(device, &create_info, descriptor_sets.data());
			return descriptor_sets;
		}

		inline VkPipelineInputAssemblyStateCreateInfo make_pipeline_input_assembly_create_info(
			VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable
		) {
			VkPipelineInputAssemblyStateCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			create_info.topology = topology;
			create_info.primitiveRestartEnable = primitiveRestartEnable;
			return create_info;
		}

		inline VkPipelineViewportStateCreateInfo make_pipeline_view_port_create_info(
			VkExtent2D swapchain_extent, VkViewport& viewport, VkRect2D& scissor
		) {

			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = (float)swapchain_extent.width;
			viewport.height = (float)swapchain_extent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			scissor.offset = {0, 0};
			scissor.extent = swapchain_extent;

			VkPipelineViewportStateCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			create_info.scissorCount = 1;
			create_info.pScissors = &scissor;
			create_info.viewportCount = 1;
			create_info.pViewports = &viewport;

			return create_info;
		}

		inline VkPipelineRasterizationStateCreateInfo make_pipeline_rasterization_create_info(

		) {
			VkPipelineRasterizationStateCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			create_info.depthClampEnable = VK_FALSE;
			create_info.rasterizerDiscardEnable = VK_FALSE;
			create_info.polygonMode = VK_POLYGON_MODE_FILL;
			create_info.cullMode = VK_CULL_MODE_BACK_BIT;
			create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
			create_info.depthBiasEnable = VK_FALSE;
			create_info.lineWidth = 1.0f;
			return create_info;
		}

		inline VkPipelineMultisampleStateCreateInfo make_pipeline_multisample_state_create_info(

		) {
			VkPipelineMultisampleStateCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			create_info.sampleShadingEnable = VK_FALSE;
			return create_info;
		}

		inline VkPipelineDepthStencilStateCreateInfo make_pipeline_depth_stencil_state_create_info(

		) {
			VkPipelineDepthStencilStateCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			create_info.depthTestEnable = VK_TRUE;
			create_info.depthWriteEnable = VK_TRUE;
			create_info.depthCompareOp = VK_COMPARE_OP_LESS;
			create_info.depthBoundsTestEnable = VK_FALSE;
			create_info.minDepthBounds = 0.0f;
			create_info.maxDepthBounds = 1.0f;
			create_info.stencilTestEnable = VK_FALSE;
			create_info.front = {};
			create_info.back = {};
			return create_info;
		}

		inline VkPipelineColorBlendAttachmentState make_pipeline_color_blend_attachment_state(

		) {
			VkPipelineColorBlendAttachmentState color_attachment{};
			color_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
											  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			color_attachment.blendEnable = VK_FALSE;
			return color_attachment;
		}

		inline VkPipelineColorBlendStateCreateInfo make_pipeline_color_blend_state_create_info(
			const VkPipelineColorBlendAttachmentState& color_blend_state
		) {
			VkPipelineColorBlendStateCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			create_info.logicOpEnable = VK_FALSE;
			create_info.attachmentCount = 1;
			create_info.pAttachments = &color_blend_state;
			return create_info;
		}

		inline VkPipelineDynamicStateCreateInfo make_pipeline_dynamic_state_create_info(
			std::vector<VkDynamicState>& dynamicStates
		) {
			dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
			VkPipelineDynamicStateCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			create_info.dynamicStateCount = dynamicStates.size();
			create_info.pDynamicStates = dynamicStates.data();
			return create_info;
		}

		inline VkPushConstantRange make_push_constant_range(

		) {
			VkPushConstantRange range{};
			range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			range.offset = 0;
			range.size = 128;
			return range;
		}

		inline VkPipelineLayoutCreateInfo make_pipeline_layout_create_info(
			const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts, VkPushConstantRange push_constant_range
		) {
			VkPipelineLayoutCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			create_info.setLayoutCount = descriptor_set_layouts.size();
			create_info.pSetLayouts = descriptor_set_layouts.data();
			create_info.pushConstantRangeCount = 1;
			create_info.pPushConstantRanges = &push_constant_range;
			return create_info;
		}

		inline VkRenderPassBeginInfo make_render_pass_begin_info(
			VkRenderPass render_pass, VkFramebuffer frame_buffer, VkExtent2D swapchain_extent,
			const std::vector<VkClearValue>& clear_colors
		) {
			VkRenderPassBeginInfo begin_info{};
			begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			begin_info.renderPass = render_pass;
			begin_info.framebuffer = frame_buffer;
			begin_info.renderArea.offset = {0, 0};
			begin_info.renderArea.extent = swapchain_extent;
			begin_info.clearValueCount = clear_colors.size();
			begin_info.pClearValues = clear_colors.data();
			return begin_info;
		}

		inline VkViewport make_draw_viewport(
			float x, float y, float width, float height, float min_depth, float max_depth
		) {
			VkViewport viewport{};
			viewport.x = x;
			viewport.y = y;
			viewport.width = width;
			viewport.height = height;
			viewport.minDepth = min_depth;
			viewport.maxDepth = max_depth;
			return viewport;
		}

		inline VkRect2D make_scissor(const VkExtent2D& swapchain_extent) {
			VkRect2D scissor{};
			scissor.offset = {0, 0};
			scissor.extent = swapchain_extent;
			return scissor;
		}

		inline VkDescriptorSetAllocateInfo make_descriptor_set_allocate_info(
			VkDescriptorPool descriptor_pool, const std::vector<VkDescriptorSetLayout>& descriptor_set_layouts
		) {
			VkDescriptorSetAllocateInfo allocate_info{};
			allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocate_info.descriptorPool = descriptor_pool;
			allocate_info.descriptorSetCount = descriptor_set_layouts.size();
			allocate_info.pSetLayouts = descriptor_set_layouts.data();
			return allocate_info;
		}

		inline VkDescriptorBufferInfo make_descriptor_buffer_info(VkBuffer buffer, uint32_t offset, uint32_t range) {
			VkDescriptorBufferInfo descriptor_buffer_info{};
			descriptor_buffer_info.buffer = buffer;
			descriptor_buffer_info.offset = offset;
			descriptor_buffer_info.range = range;
			return descriptor_buffer_info;
		}
	} // namespace Structs

} // namespace Vulkan