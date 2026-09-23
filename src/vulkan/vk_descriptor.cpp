#include <log.h>
#include <vulkan/vk_consts.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_descriptor.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

	Descriptor_Set_Layout_Builder& Descriptor_Set_Layout_Builder::add_binding(
		uint32_t binding, VkDescriptorType type, uint32_t count, VkShaderStageFlags stage_flags
	) {
		bindings.push_back({binding, type, count, stage_flags, nullptr});
		return *this;
	}

	VkDescriptorSetLayout Descriptor_Set_Layout_Builder::build(VkDevice device) {
		if (device == VK_NULL_HANDLE) {
			device = Vulkan::device;
		}
		if (device == VK_NULL_HANDLE) {
			throw std::runtime_error("Vulkan fail to build descriptor set layout: try to init device first!");
		}
		VkDescriptorSetLayoutCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		create_info.bindingCount = bindings.size();
		create_info.pBindings = bindings.data();
		VkDescriptorSetLayout layout{};
		Utils::vk_check_result(
			vkCreateDescriptorSetLayout(device, &create_info, nullptr, &layout), "",
			"Vulkan fail to create descriptor set layout!"
		);
		return layout;
	}

	Descriptor_Set_Writer& Descriptor_Set_Writer::add_buffer_write(
		uint32_t binding, VkDescriptorBufferInfo* descriptor_buffer_info, VkDescriptorSet dst_set
	) {
		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = dst_set;
		write.dstBinding = binding;
		write.dstArrayElement = 0;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.pBufferInfo = descriptor_buffer_info;
		writes.push_back(write);
		return *this;
	}

	Descriptor_Set_Writer& Descriptor_Set_Writer::add_image_write(
		uint32_t binding, VkDescriptorImageInfo* descriptor_image_info, VkDescriptorSet dst_set
	) {
		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = dst_set;
		write.dstBinding = binding;
		write.dstArrayElement = 0;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		write.pImageInfo = descriptor_image_info;
		writes.push_back(write);
		return *this;
	}

	void Descriptor_Set_Writer::write(VkDevice device) {
		if (device == VK_NULL_HANDLE) {
			device = Vulkan::device;
		}
		if (device == VK_NULL_HANDLE) {
			throw std::runtime_error("Vulkan fail to write descriptor set: try to init device first!");
		}
		vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
	}

	namespace Init {

		void _init_descriptor_pools() {

			std::vector<VkDescriptorPoolSize> pool_size{};
			pool_size.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10000});
			pool_size.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10000});
			pool_size.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10000});

			descriptor_pools.reserve(Const::MAX_FRAMES_IN_FLIGHT);
			for (int i = 0; i < Const::MAX_FRAMES_IN_FLIGHT; i++) {

				VkDescriptorPoolCreateInfo create_info{};
				create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				create_info.poolSizeCount = pool_size.size();
				create_info.pPoolSizes = pool_size.data();
				create_info.maxSets = 50;

				VkDescriptorPool pool{};
				Utils::vk_check_result(
					vkCreateDescriptorPool(device, &create_info, nullptr, &pool),
					"Vulkan create descriptor pool successfully!", "Vulkan fail to create descriptor pool!"
				);
				descriptor_pools.emplace_back(pool);
			}
		}

	} // namespace Init

	namespace Destroy {

		void _destroy_descriptor_pools() {

			for (auto& pool : descriptor_pools) {
				vkDestroyDescriptorPool(device, pool, nullptr);
			}

			Log::log_info("Vulkan destroy descriptor pools successfully!");
		}
	} // namespace Destroy

} // namespace Vulkan