#pragma once
#include <vector>
#include <vulkan/vulkan.h>

namespace Vulkan {

	struct Descriptor_Set_Layout_Builder {

		std::vector<VkDescriptorSetLayoutBinding> bindings;

		Descriptor_Set_Layout_Builder& add_binding(
			uint32_t binding, VkDescriptorType type, uint32_t count, VkShaderStageFlags stage_flags
		);

		VkDescriptorSetLayout build(VkDevice device);

		void clear();
	};

	struct Descriptor_Set_Writer {

		std::vector<VkWriteDescriptorSet> writes;

		Descriptor_Set_Writer& add_buffer_write(
			uint32_t binding, VkDescriptorBufferInfo* descriptor_buffer_info, VkDescriptorSet dst_set
		);

		Descriptor_Set_Writer& add_image_write(
			uint32_t binding, int image_count, VkDescriptorImageInfo* descriptor_image_info, VkDescriptorSet dst_set
		);

		void write(VkDevice device);

		void clear();
	};

	void init_descriptor_pools(std::vector<VkDescriptorPool>& descriptor_pools, VkDevice device);

	void destroy_descriptor_pools(const std::vector<VkDescriptorPool>& descriptor_pools, VkDevice device);

} // namespace Vulkan