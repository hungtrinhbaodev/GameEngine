#pragma once
#include <vulkan/vulkan.h>

#include <vector>
#include <string>
#include <stdexcept>

#include <vulkan/vk_image.h>

namespace Vulkan {

	struct Pipeline_Config {

		VkDevice device;

		VkRenderPass render_pass;

		VkExtent2D swapchain_extent;

		std::vector<VkDescriptorSetLayout> descriptor_set_layouts;

		std::string vertex_shader_path;

		std::string fragment_shader_path;

		Image depth_image;

		std::vector<VkVertexInputBindingDescription> vertex_descriptions;

		std::vector<VkVertexInputAttributeDescription> attribute_descriptions;
	};

	struct Pipeline {

		VkPipeline pipeline;

		VkPipelineLayout layout;

		std::vector<VkDescriptorSetLayout> descriptor_set_layouts;

		void init(const Pipeline_Config& config);

		void destroy(VkDevice device = VK_NULL_HANDLE) const;
	};

} // namespace Vulkan