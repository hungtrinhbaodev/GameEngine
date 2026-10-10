#include <GLFW/glfw3.h>
#include <map>
#include <vulkan/vk_command_pool.h>
#include <vulkan/vk_descriptor.h>
#include <vulkan/vk_fences.h>
#include <vulkan/vk_queues.h>
#include <vulkan/vk_structs.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {
	namespace Utils {

		std::unordered_map<VkDevice, glm::vec2> window_sizes{};

		void save_window_size(GLFWwindow* window, VkDevice device) {
			int width, height;
			glfwGetWindowSize(window, &width, &height);
			window_sizes[device] = {(float)width, (float)height};
		}

		glm::vec2 get_window_size(VkDevice device) {
			if (window_sizes.find(device) == window_sizes.end()) {
				throw std::runtime_error(
					"Fail to get wihdow size, please save_window_size for this " + std::to_string((uint64_t)device) +
					"!"
				);
			}
			return window_sizes[device];
		}

		void copy_data_to_multi_buffer(
			VkBuffer src_buffer, const std::map<VkBuffer, std::vector<VkBufferCopy>>& copied_data, VkDevice device
		) {
			VkCommandBuffer command_buffer = start_commands(device);
			for (auto& [dst_buffer, copied_ranges] : copied_data) {
				vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, copied_ranges.size(), copied_ranges.data());
			}
			finish_commands(device, command_buffer);
		}

		VkCommandBuffer start_commands(VkDevice device) {
			VkCommandBuffer command_buffer = request_command_buffer(device);
			VkCommandBufferBeginInfo begin_info = Structs::make_command_begin_info();
			vkBeginCommandBuffer(command_buffer, &begin_info);
			return command_buffer;
		}

		void finish_commands(VkDevice device, VkCommandBuffer command_buffer) {
			vkEndCommandBuffer(command_buffer);
			VkFence fence = request_fence(device);
			VkSubmitInfo submit_info = Structs::make_submit_info(&command_buffer);
			auto success = [](VkDevice device, VkCommandBuffer command_buffer, VkFence fence) {
				release_command_buffer(device, command_buffer);
				release_fence(device, fence);
			};
			submit(submit_info, device, fence);
			on_fence_success(device, fence, success, device, command_buffer, fence).get();
		}

		std::vector<VkDescriptorSet> make_texture_descriptor_sets(
			const std::vector<VkDescriptorPool>& descriptor_pools, VkDescriptorSetLayout layout,
			VkDescriptorImageInfo image_descriptor, VkDevice device
		) {
			std::vector<VkDescriptorSet> texture_descriptor_sets =
				Structs::make_descriptor_sets(descriptor_pools, &layout, device);
			Descriptor_Set_Writer writer{};
			for (int i = 0; i < texture_descriptor_sets.size(); i++) {
				writer.add_image_write(0, 1, &image_descriptor, texture_descriptor_sets[i]).write(device);
				writer.clear();
			}
			return texture_descriptor_sets;
		}

		VkDescriptorSet make_buffer_descriptor_set(
			VkDescriptorPool descriptor_pool, VkDescriptorSetLayout layout, VkDescriptorBufferInfo buffer_descriptor,
			VkDevice device
		) {
			VkDescriptorSet descriptor_set = Structs::make_descriptor_set(descriptor_pool, &layout, device);
			Descriptor_Set_Writer writer{};
			writer.add_buffer_write(0, &buffer_descriptor, descriptor_set).write(device);
			return descriptor_set;
		}
	} // namespace Utils
} // namespace Vulkan