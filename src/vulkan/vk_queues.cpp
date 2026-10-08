#include <log.h>
#include <mutex>
#include <vulkan/queue_family_indices.h>
#include <vulkan/vk_queues.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

	std::mutex _submit_mutex;

	namespace Init {
		void _init_queues(
			VkQueue& graphics_queue, VkQueue& present_queue, VkSurfaceKHR surface, VkPhysicalDevice physical_device,
			VkDevice device
		) {
			Queue_Family_Indices indices = Utils::query_suitable_queue_family_indices(physical_device, surface);

			vkGetDeviceQueue(device, indices.graphic_family.value(), 0, &graphics_queue);
			Log::info("Get device graphic queue at index", indices.graphic_family.value(), graphics_queue, "success!");

			vkGetDeviceQueue(device, indices.present_family.value(), 0, &present_queue);
			Log::info("Get device present queue at index", indices.present_family.value(), present_queue, "success!");
		}

	} // namespace Init

	namespace API {
		void submit(const VkSubmitInfo& submit_info, VkQueue submit_queue, VkFence fence) {
			std::lock_guard<std::mutex> lock(_submit_mutex);
			vkQueueSubmit(submit_queue, 1, &submit_info, fence);
		}

		VkResult submit_present(const VkPresentInfoKHR& present_info, VkQueue present_queue) {
			VkResult submit_result = VK_INCOMPLETE;
			{
				std::lock_guard<std::mutex> lock(_submit_mutex);
				submit_result = vkQueuePresentKHR(present_queue, &present_info);
			}
			return submit_result;
		}

	} // namespace API

} // namespace Vulkan