#include <vulkan/vk_vertex_input_builder.h>
#include <vulkan/vk_structs.h>

namespace Vulkan {

	Vertex_Input_Builder& Vertex_Input_Builder::add_binding_description(uint32_t binding, uint32_t stride,
																		VkVertexInputRate input_rate) {
		binding_descriptions.push_back({binding, stride, input_rate});
		if (binding >= attribute_descriptions_by_bindings.size()) {
			attribute_descriptions_by_bindings.resize(static_cast<size_t>(binding + 1));
		}
		return *this;
	}

	Vertex_Input_Builder& Vertex_Input_Builder::add_attribute_description(uint32_t binding, VkFormat format,
																		  uint32_t offset) {
		if (binding >= attribute_descriptions_by_bindings.size()) {
			throw std::runtime_error("Binding index out of range, please add more first!");
		}
		attribute_descriptions_by_bindings[binding].push_back({0, binding, format, offset});
		return *this;
	}

	Vertex_Input_Builder& Vertex_Input_Builder::add_mat4_attribute_description(uint32_t binding, uint32_t offset) {
		if (binding >= attribute_descriptions_by_bindings.size()) {
			throw std::runtime_error("Binding index out of range, please add more first!");
		}
		for (int i = 0; i < 4; i++) {
			attribute_descriptions_by_bindings[binding].push_back(
				{0, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offset + static_cast<uint32_t>(sizeof(float)) * 4 * i});
		}
		return *this;
	}

	VkPipelineVertexInputStateCreateInfo Vertex_Input_Builder::build() {
		std::vector<VkVertexInputAttributeDescription> all_attribute_descriptions;
		uint32_t location_counter = 0;
		for (auto& attribute_descriptions : attribute_descriptions_by_bindings) {
			for (auto& attribute_description : attribute_descriptions) {
				attribute_description.location = location_counter;
				all_attribute_descriptions.push_back(attribute_description);
				location_counter++;
			}
		}
		return Structs::make_pipeline_vertex_input_state_create_info(binding_descriptions, all_attribute_descriptions);
	}
} // namespace Vulkan