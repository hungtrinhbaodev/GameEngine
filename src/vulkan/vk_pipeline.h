#pragma once
#include <vulkan/vulkan.h>

#include <stdexcept>
#include <string>
#include <vector>

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

		std::vector<VkVertexInputBindingDescription> vertex_binding_descriptions;

		std::vector<VkVertexInputAttributeDescription> attribute_descriptions;

		uint32_t push_constants_size = 128;

		bool enable_depth_image = false;

		VkCompareOp depth_compare_op = VK_COMPARE_OP_LESS;

		VkFrontFace triangle_trip_order = VK_FRONT_FACE_CLOCKWISE;
	};

	struct Pipeline {

		VkDevice device = VK_NULL_HANDLE;

		VkPipeline pipeline = VK_NULL_HANDLE;

		VkPipelineLayout layout = VK_NULL_HANDLE;

		void init(const Pipeline_Config& config);

		void destroy() const;
	};

} // namespace Vulkan