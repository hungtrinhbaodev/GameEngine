#include <GLFW/glfw3.h>
#include <map>
#include <vulkan/vk_command_pool.h>
#include <vulkan/vk_common.h>
#include <vulkan/vk_descriptor.h>
#include <vulkan/vk_fences.h>
#include <vulkan/vk_queues.h>
#include <vulkan/vk_structs.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {
	namespace Utils {

		std::unordered_map<VkDevice, glm::vec2> window_sizes{};

		std::unordered_map<VkDevice, Draw_2D_Common_Data> draw_2D_configs{};

		std::unordered_map<VkDevice, std::pair<uint32_t, uint32_t>> draw_2D_config_storage{};

		void save_window_size(GLFWwindow* window, VkDevice device) {
			int width, height;
			glfwGetWindowSize(window, &width, &height);
			window_sizes[device] = {(float)width, (float)height};
		}

		void save_window_size(GLFWwindow* window, VkDevice device, SSBO_Buffer& ssbo) {
			if (draw_2D_configs.find(device) == draw_2D_configs.end()) {
				draw_2D_configs[device] = {};
				draw_2D_config_storage[device] = {};
				auto& storage_info = draw_2D_config_storage[device];
				storage_info.first = ssbo.upload_data(
					&draw_2D_configs[device], sizeof(Draw_2D_Common_Data), sizeof(Draw_2D_Common_Data)
				);
				const Buffer_Range& config_range = ssbo.view_slot(storage_info.first);
				storage_info.second = config_range.offset_as<Draw_2D_Common_Data>();
			}
			int width, height;
			glfwGetWindowSize(window, &width, &height);
			draw_2D_configs[device].screen_size = {(float)width, (float)height};
			ssbo.update_data(draw_2D_config_storage[device].first, &draw_2D_configs[device]);
		}

		uint32_t get_draw_2D_config_data_offset(VkDevice device) {
			return draw_2D_config_storage[device].second;
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
			record_copy_data_to_multi_buffer(command_buffer, src_buffer, copied_data);
			finish_commands(device, command_buffer);
		}

		void record_copy_data_to_multi_buffer(
			VkCommandBuffer command_buffer, VkBuffer src_buffer,
			const std::map<VkBuffer, std::vector<VkBufferCopy>>& copied_data
		) {
			for (auto& [dst_buffer, copied_ranges] : copied_data) {
				vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, copied_ranges.size(), copied_ranges.data());
			}
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

		std::array<VkDescriptorSet, Const::MAX_FRAMES_IN_FLIGHT> make_texture_descriptor_sets_2(
			const std::array<VkDescriptorPool, Const::MAX_FRAMES_IN_FLIGHT>& descriptor_pools,
			VkDescriptorSetLayout layout, VkDescriptorImageInfo image_descriptor, VkDevice device
		) {
			std::array<VkDescriptorSet, Const::MAX_FRAMES_IN_FLIGHT> texture_descriptor_sets =
				Structs::make_descriptor_sets_2(descriptor_pools, &layout, device);
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

		uint32_t calculate_mip_level(uint32_t width, uint32_t height) {
			return (uint32_t)(std::log2(std::max(width, height))) + 1;
		}

		bool can_gpu_blit_image(VkPhysicalDevice physical_device, VkFormat format, VkFilter filter) {
			VkFormatProperties props;
			VkFormatFeatureFlags require = VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT;
			vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);
			if (filter == VK_FILTER_LINEAR) {
				require |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
			}
			return (props.optimalTilingFeatures & require) == require;
		}

		Buffer_Range find_suitable_buffer_range(
			std::priority_queue<Buffer_Range, std::vector<Buffer_Range>, Buffer_Range_Compare>& available_ranges,
			uint32_t& current_offset, uint32_t size, uint32_t element_stride
		) {
			Buffer_Range using_range{0, 0, 0};
			if (available_ranges.size() > 0 && available_ranges.top().is_suitable_with(size, element_stride)) {
				Buffer_Range optimal_range = available_ranges.top();
				available_ranges.pop();
				Buffer_Range remain_range = optimal_range.fit_with(size, element_stride);
				available_ranges.push(remain_range);
				using_range = optimal_range;
			} else {
				uint32_t align = calculate_range_remain_size(current_offset, element_stride);
				if (align > 0) {
					available_ranges.push({current_offset, 0, align, 1});
				}
				current_offset += align;
				using_range = {current_offset, 0, size, element_stride};
				current_offset += using_range.total_size();
			}
			return using_range;
		}

		uint32_t calculate_range_remain_size(uint32_t offset, uint32_t element_stride) {
			return (element_stride - (offset % element_stride)) % element_stride;
		}

		void record_buffer_barrier(
			VkCommandBuffer command_buffer, VkAccessFlags src_access, VkAccessFlags dst_access,
			VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage, VkBuffer buffer, uint32_t offset,
			uint32_t size
		) {
			VkBufferMemoryBarrier barrier = Structs::make_buffer_barier(src_access, dst_access, buffer, offset, size);
			vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, nullptr, 1, &barrier, 0, nullptr);
		}
	} // namespace Utils
} // namespace Vulkan