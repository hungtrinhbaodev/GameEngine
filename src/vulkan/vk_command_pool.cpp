#include <concurrent_pool.h>
#include <core.h>
#include <exception>
#include <functional>
#include <log.h>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vulkan/vk_command_pool.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

	struct Command_Pool_Infomation {
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkPhysicalDevice physical_device = VK_NULL_HANDLE;
		VkDevice device = VK_NULL_HANDLE;
	};

	class Command_Pool_Thread : public Concurent_Pool<VkCommandBuffer> {
	  private:
		Command_Pool_Infomation information{};
		VkCommandPool _command_pool;
		VkCommandBuffer _create_item() override {
			VkCommandBuffer command_buffer = VK_NULL_HANDLE;
			VkCommandBufferAllocateInfo allocate_info{};
			allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocate_info.commandPool = _command_pool;
			allocate_info.commandBufferCount = 1;

			Utils::vk_check_result(
				vkAllocateCommandBuffers(information.device, &allocate_info, &command_buffer), "",
				"Vulkan fail to create command buffer!"
			);

			return command_buffer;
		}
		void _delete_item(VkCommandBuffer& command_buffer) override { vkResetCommandBuffer(command_buffer, 0); }

	  public:
		Command_Pool_Thread(Command_Pool_Infomation information) { this->information = information; }
		void init_pool() {

			Queue_Family_Indices indices =
				Utils::query_suitable_queue_family_indices(information.physical_device, information.surface);

			VkCommandPoolCreateInfo pool_info{};
			pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			pool_info.queueFamilyIndex = indices.graphic_family.value();

			// init vulkan command pool
			Utils::vk_check_result(
				vkCreateCommandPool(information.device, &pool_info, nullptr, &_command_pool), "",
				"Vulkan fail to create command pool!"
			);

			Log::info(
				"Vulkan create command pool at thread at device", information.device, std::this_thread::get_id(),
				_command_pool, " successfully!"
			);
		}
		void destroy() {
			Concurent_Pool<VkCommandBuffer>::destroy();

			vkDestroyCommandPool(information.device, _command_pool, nullptr);

			Log::info("Vulkan delete command pool at thread", std::this_thread::get_id(), "successfully!");
		}
	};

	std::unordered_map<VkDevice, std::unordered_map<uint64_t, std::shared_ptr<Command_Pool_Thread>>>
		command_pool_threads{};

	std::mutex commands_mutex{};

	std::unordered_map<VkCommandBuffer, uint64_t> command_to_thread_ids{};

	std::shared_ptr<Command_Pool_Thread> get_command_thread_pool(VkDevice device) {

		auto thread_id = std::this_thread::get_id();
		uint64_t hash_thread_id = std::hash<std::thread::id>()(thread_id);

		if (command_pool_threads.find(device) == command_pool_threads.end()) {
			throw std::runtime_error(
				"Vulkan can't find device " + std::to_string((uint64_t)device) + " init command pool!"
			);
		}

		if (command_pool_threads[device].find(hash_thread_id) == command_pool_threads[device].end()) {
			throw std::runtime_error("Vulkan fail to find command pool at thread!");
		}

		return command_pool_threads[device][hash_thread_id];
	}

	void init_command_pool_threads(VkSurfaceKHR surface, VkPhysicalDevice physical_device, VkDevice device) {
		Command_Pool_Infomation information{surface, physical_device, device};
		command_pool_threads[device] = {};
		std::shared_ptr<std::mutex> init_pool_lock = std::make_shared<std::mutex>();

		auto create_command_pool_thread = [](std::shared_ptr<std::mutex> init_pool_lock,
											 Command_Pool_Infomation information) {
			auto thread_id = std::this_thread::get_id();
			uint64_t hash_thread_id = std::hash<std::thread::id>()(thread_id);
			{
				std::lock_guard<std::mutex> lock(*init_pool_lock);
				command_pool_threads[information.device].emplace(
					hash_thread_id, std::make_shared<Command_Pool_Thread>(information)
				);
				command_pool_threads[information.device][hash_thread_id]->init_pool();
			}
		};

		auto results =
			Core::global_thread_pool->loop_all_threads(create_command_pool_thread, init_pool_lock, information);

		for (auto& [_, result] : results) {
			result.get();
		}

		create_command_pool_thread(init_pool_lock, information);
	}

	void destroy_command_pool_threads(VkDevice device) {

		std::shared_ptr<std::mutex> destroy_pool_lock = std::make_shared<std::mutex>();
		auto destroy_command_pool_thread = [](std::shared_ptr<std::mutex> destroy_pool_lock, VkDevice device) {
			auto thread_id = std::this_thread::get_id();
			uint64_t hash_thread_id = std::hash<std::thread::id>()(thread_id);
			{
				std::lock_guard<std::mutex> lock(*destroy_pool_lock);
				command_pool_threads[device][hash_thread_id]->destroy();
			}
		};
		auto results =
			Core::global_thread_pool->loop_all_threads(destroy_command_pool_thread, destroy_pool_lock, device);

		for (auto& [_, result] : results) {
			result.get();
		}
		destroy_command_pool_thread(destroy_pool_lock, device);
	}

	VkCommandBuffer request_command_buffer(VkDevice device) {
		VkCommandBuffer command_buffer = get_command_thread_pool(device)->request_item();
		{
			std::unique_lock<std::mutex> lock(commands_mutex);
			if (command_to_thread_ids.find(command_buffer) == command_to_thread_ids.end()) {
				auto thread_id = std::this_thread::get_id();
				uint64_t hash_thread_id = std::hash<std::thread::id>()(thread_id);
				command_to_thread_ids[command_buffer] = hash_thread_id;
			}
		}
		vkResetCommandBuffer(command_buffer, 0);
		return command_buffer;
	}

	void release_command_buffer(VkDevice device, VkCommandBuffer& command_buffer) {
		uint64_t hash_thread_id = 0;
		{
			std::unique_lock<std::mutex> lock(commands_mutex);
			hash_thread_id = command_to_thread_ids[command_buffer];
		}
		if (command_pool_threads.find(device) == command_pool_threads.end()) {
			throw std::runtime_error(
				"Vulkan can't find device " + std::to_string((uint64_t)device) + " init command pool!"
			);
		}
		if (command_pool_threads[device].find(hash_thread_id) == command_pool_threads[device].end()) {
			throw std::runtime_error("Vulkan fail to release command buffer: can't find command pool at thread!");
		}
		return command_pool_threads[device][hash_thread_id]->pooling_item(command_buffer);
	}

} // namespace Vulkan