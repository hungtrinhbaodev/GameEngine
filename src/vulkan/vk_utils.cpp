#include <vulkan/vk_core.h>
#include <vulkan/vk_utils.h>

#include <vulkan/vk_command_pool.h>
#include <vulkan/vk_fences.h>
#include <vulkan/vk_queues.h>
#include <vulkan/vk_structs.h>

namespace Vulkan {
	namespace Utils {

		float window_size_width = 0.f;
		float window_size_height = 0.f;

		void save_window_size() {
			int width, height;
			glfwGetWindowSize(Vulkan::_window, &width, &height);
			window_size_width = (float)width;
			window_size_height = (float)height;
		}

		glm::vec2 get_window_size() {
			return glm::vec2(window_size_width, window_size_height);
		}

		void copy_data_to_multi_buffer(
			VkBuffer src_buffer, const std::map<VkBuffer, std::vector<VkBufferCopy>>& copied_data
		) {
			VkCommandBuffer command_buffer = start_commands();
			for (auto& [dst_buffer, copied_ranges] : copied_data) {
				vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, copied_ranges.size(), copied_ranges.data());
			}
			finish_commands(command_buffer);
		}

		VkCommandBuffer start_commands() {
			std::thread::id thread_id = std::this_thread::get_id();
			VkCommandBuffer command_buffer = API::request_command_buffer();

			VkCommandBufferBeginInfo begin_info = Structs::make_command_begin_info();
			vkBeginCommandBuffer(command_buffer, &begin_info);
			return command_buffer;
		}

		void finish_commands(VkCommandBuffer command_buffer) {
			vkEndCommandBuffer(command_buffer);
			VkFence fence = API::request_fence();
			VkSubmitInfo submit_info = Structs::make_submit_info(&command_buffer);
			auto success = [](VkCommandBuffer command_buffer, VkFence fence) {
				API::release_command_buffer(command_buffer);
				API::release_fence(fence);
			};
			API::submit(submit_info, Vulkan::graphics_queue, fence);
			API::on_fence_success(fence, success, command_buffer, fence).get();
		}
	} // namespace Utils
} // namespace Vulkan