#pragma once

#include <functional>
#include <thread>
#include <unordered_map>

#include <ThreadPool.h>
#include <vulkan/vulkan.h>

#include <concurrent_pool.h>

namespace Vulkan {

	class _Command_Pool_Thread : public Concurent_Pool<VkCommandBuffer> {

	  private:
		VkCommandPool _command_pool;

		VkCommandBuffer _create_item() override;

		void _delete_item(VkCommandBuffer& command_buffer) override;

	  public:
		void init_pool();

		void destroy();
	};

	inline std::unordered_map<uint64_t, std::shared_ptr<_Command_Pool_Thread>> _command_pool_threads;

	std::shared_ptr<_Command_Pool_Thread> _get_command_thread_pool();

	namespace Init {
		void _init_command_pool_threads();
	}

	namespace Destroy {
		void _destroy_command_pool_threads();
	}

	namespace API {
		VkCommandBuffer request_command_buffer();

		void release_command_buffer(VkCommandBuffer& command_buffer, std::thread::id thread_id);

	} // namespace API

} // namespace Vulkan