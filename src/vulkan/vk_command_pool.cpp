#include <functional>
#include <vulkan/vk_core.h>
#include <vulkan/vk_command_pool.h>
#include <vulkan/vk_utils.h>
#include <exception>
#include <log.h>

namespace Vulkan {

	VkCommandBuffer _Command_Pool_Thread::_create_item() {

		VkCommandBuffer command_buffer = VK_NULL_HANDLE;
		VkCommandBufferAllocateInfo allocate_info{};
		allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocate_info.commandPool = _command_pool;
		allocate_info.commandBufferCount = 1;

		Utils::vk_check_result(
			vkAllocateCommandBuffers(device, &allocate_info, &command_buffer), "",
			"Vulkan fail to create command buffer!"
		);

		return command_buffer;
	}

	void _Command_Pool_Thread::_delete_item(VkCommandBuffer& command_buffer) {

		vkResetCommandBuffer(command_buffer, 0);
	}

	void _Command_Pool_Thread::init_pool() {

		Queue_Family_Indices indices = Utils::query_suitable_queue_family_indices(physical_device, surface);

		VkCommandPoolCreateInfo pool_info{};
		pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		pool_info.queueFamilyIndex = indices.graphic_family.value();

		// init vulkan command pool
		Utils::vk_check_result(
			vkCreateCommandPool(device, &pool_info, nullptr, &_command_pool), "", "Vulkan fail to create command pool!"
		);

		Log::log_info(
			"Vulkan create command pool at thread", std::this_thread::get_id(), _command_pool, " successfully!"
		);
	}

	void _Command_Pool_Thread::destroy() {

		Concurent_Pool<VkCommandBuffer>::destroy();

		vkDestroyCommandPool(device, _command_pool, nullptr);

		Log::log_info("Vulkan delete command pool at thread", std::this_thread::get_id(), "successfully!");
	}

	std::shared_ptr<_Command_Pool_Thread> _get_command_thread_pool() {

		auto thread_id = std::this_thread::get_id();
		uint64_t hash_thread_id = std::hash<std::thread::id>()(thread_id);

		if (_command_pool_threads.find(hash_thread_id) == _command_pool_threads.end()) {
			throw std::runtime_error("Vulkan fail to find command pool at thread!");
		}

		return _command_pool_threads[hash_thread_id];
	}

	namespace Init {

		void _init_command_pool_threads() {

			std::shared_ptr<std::mutex> init_pool_lock = std::make_shared<std::mutex>();

			auto create_command_pool_thread = [](std::shared_ptr<std::mutex> init_pool_lock) {
				auto thread_id = std::this_thread::get_id();
				uint64_t hash_thread_id = std::hash<std::thread::id>()(thread_id);
				{
					std::lock_guard<std::mutex> lock(*init_pool_lock);
					_command_pool_threads.emplace(hash_thread_id, std::make_shared<_Command_Pool_Thread>());
					_command_pool_threads[hash_thread_id]->init_pool();
				}
			};

			auto results = _global_thread_pool->loop_all_threads(create_command_pool_thread, init_pool_lock);

			for (auto& [_, result] : results) {
				result.get();
			}

			create_command_pool_thread(init_pool_lock);
		}
	} // namespace Init

	namespace Destroy {

		void _destroy_command_pool_threads() {

			std::shared_ptr<std::mutex> destroy_pool_lock = std::make_shared<std::mutex>();
			auto destroy_command_pool_thread = [](std::shared_ptr<std::mutex> destroy_pool_lock) {
				auto thread_id = std::this_thread::get_id();
				uint64_t hash_thread_id = std::hash<std::thread::id>()(thread_id);
				{
					std::lock_guard<std::mutex> lock(*destroy_pool_lock);
					_command_pool_threads[hash_thread_id]->destroy();
				}
			};
			auto results = _global_thread_pool->loop_all_threads(destroy_command_pool_thread, destroy_pool_lock);

			for (auto& [_, result] : results) {
				result.get();
			}

			destroy_command_pool_thread(destroy_pool_lock);
		}

	} // namespace Destroy

	namespace API {

		VkCommandBuffer request_command_buffer() {
			VkCommandBuffer command_buffer = _get_command_thread_pool()->request_item();
			vkResetCommandBuffer(command_buffer, 0);
			return command_buffer;
		}

		void release_command_buffer(VkCommandBuffer& command_buffer, std::thread::id thread_id) {
			uint64_t hash_thread_id = std::hash<std::thread::id>()(thread_id);
			if (_command_pool_threads.find(hash_thread_id) == _command_pool_threads.end()) {
				throw std::runtime_error("Vulkan fail to release command buffer: can't find command pool at thread!");
			}
			return _command_pool_threads[hash_thread_id]->pooling_item(command_buffer);
		}

	} // namespace API

} // namespace Vulkan