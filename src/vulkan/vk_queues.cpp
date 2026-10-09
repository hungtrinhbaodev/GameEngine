#include <log.h>
#include <map>
#include <mutex>
#include <vulkan/queue_family_indices.h>
#include <vulkan/vk_queues.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

	struct Queue_Information {
		VkQueue graphics_queue = VK_NULL_HANDLE;
		VkQueue present_queue = VK_NULL_HANDLE;
	};

	std::unordered_map<VkDevice, Queue_Information> queues{};

	std::unordered_map<VkDevice, std::shared_ptr<std::mutex>> submit_locks{};

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

			queues[device] = {graphics_queue, present_queue};
			submit_locks[device] = std::make_shared<std::mutex>();
		}

	} // namespace Init

	namespace API {
		void submit(const VkSubmitInfo& submit_info, VkDevice device, VkFence fence) {
			if (submit_locks.find(device) == submit_locks.end()) {
				throw std::runtime_error(
					"Fail to submit graphic command with device please try _init_queues with this " +
					std::to_string((uint64_t)device) + "!"
				);
			}
			std::lock_guard<std::mutex> lock(*submit_locks[device]);
			vkQueueSubmit(queues[device].graphics_queue, 1, &submit_info, fence);
		}

		VkResult submit_present(const VkPresentInfoKHR& present_info, VkDevice device) {
			VkResult submit_result = VK_INCOMPLETE;
			{
				if (submit_locks.find(device) == submit_locks.end()) {
					throw std::runtime_error(
						"Fail to submit present command with device please try _init_queues with this " +
						std::to_string((uint64_t)device) + "!"
					);
				}
				std::lock_guard<std::mutex> lock(*submit_locks[device]);
				submit_result = vkQueuePresentKHR(queues[device].present_queue, &present_info);
			}
			return submit_result;
		}

	} // namespace API

} // namespace Vulkan