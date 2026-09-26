#pragma once
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>

namespace Vulkan {

	struct Vertex_Input_Builder {

		std::vector<VkVertexInputBindingDescription> binding_descriptions;

		std::vector<std::vector<VkVertexInputAttributeDescription>> attribute_descriptions_by_bindings;

		Vertex_Input_Builder& add_binding_description(uint32_t binding, uint32_t stride, VkVertexInputRate input_rate);

		Vertex_Input_Builder& add_attribute_description(uint32_t binding, VkFormat format, uint32_t offset);

		Vertex_Input_Builder& add_mat4_attribute_description(uint32_t binding, uint32_t offset);

		Vertex_Input_Builder& add_array_vec2_attribute_description(
			uint32_t binding, uint32_t offset, uint32_t array_size
		);

		std::vector<VkVertexInputBindingDescription> build_binding_descriptions();

		std::vector<VkVertexInputAttributeDescription> build_attribute_descriptions();
	};
} // namespace Vulkan