#include <vulkan/vk_core.h>
#include <vulkan/vk_utils.h>

#include <vulkan/vk_command_pool.h>
#include <vulkan/vk_fences.h>
#include <vulkan/vk_queues.h>
#include <vulkan/vk_structs.h>

namespace Vulkan {
	namespace Utils {
		glm::vec2 get_window_size() {
			int width, height;
			glfwGetWindowSize(Vulkan::_window, &width, &height);
			return glm::vec2(width, height);
		}

		void copy_data_to_multi_buffer(
			VkBuffer src_buffer, const std::map<VkBuffer, std::vector<VkBufferCopy>>& copied_data
		) {
			std::thread::id thread_id = std::this_thread::get_id();
			VkCommandBuffer command_buffer = API::request_command_buffer();
			VkFence fence = API::request_fence();

			VkCommandBufferBeginInfo begin_info = Structs::make_command_begin_info();
			vkBeginCommandBuffer(command_buffer, &begin_info);
			for (auto& [dst_buffer, copied_ranges] : copied_data) {
				vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, copied_ranges.size(), copied_ranges.data());
			}
			vkEndCommandBuffer(command_buffer);
			VkSubmitInfo submit_info = Structs::make_submit_info(&command_buffer);
			auto success = [](std::thread::id thread_id, VkCommandBuffer command_buffer, VkFence fence) {
				API::release_command_buffer(command_buffer, thread_id);
				API::release_fence(fence);
			};
			API::submit(submit_info, fence);
			API::on_fence_success(fence, success, thread_id, command_buffer, fence).get();
		}
	} // namespace Utils
} // namespace Vulkan