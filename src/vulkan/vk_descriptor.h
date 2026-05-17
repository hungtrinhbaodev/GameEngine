#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace Vulkan {

	struct Descriptor_Set_Layout_Builder {

		std::vector<VkDescriptorSetLayoutBinding> bindings;

		Descriptor_Set_Layout_Builder& add_binding(
			uint32_t binding,
			VkDescriptorType type,
			uint32_t count,
			VkShaderStageFlags
			stage_flags
		);
		 
		VkDescriptorSetLayout build(VkDevice device = VK_NULL_HANDLE);

	};
	
	struct Descriptor_Set_Writer {

		std::vector<VkWriteDescriptorSet> writes;

		Descriptor_Set_Writer& add_uniform_buffer_write(uint32_t binding, VkDescriptorBufferInfo* descriptor_buffer_info, VkDescriptorSet dst_set);

		Descriptor_Set_Writer& add_image_write(uint32_t binding, VkDescriptorImageInfo* descriptor_image_info, VkDescriptorSet dst_set);

		void write(VkDevice device = VK_NULL_HANDLE);
	};

	namespace Init {

		void _init_descriptor_pools();

	}

	namespace Destroy {

		void _destroy_descriptor_pools();

	}

}