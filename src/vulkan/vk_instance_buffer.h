#pragma once
#include <vulkan/vulkan.h>

#include <vulkan/vk_ring_buffer.h>

#include <map>
#include <set>
#include <stack>

namespace Vulkan {

	struct Instance_Buffer {

		uint32_t available_size = 0;

		uint32_t number_instance = 0;

		Buffer inner_buffer;

		std::stack<uint32_t> available_id;

		std::vector<char> instances_data;

		std::set<uint32_t> update_indices;

		std::map<uint32_t, uint32_t> ids_to_indices;

		std::map<uint32_t, uint32_t> indices_to_ids;

		uint32_t counter_id = 0;

		// Stride is size of one instance type data example: sizeof(MeshData)
		uint32_t stride = 0;

		Ring_Buffer* staging_buffer = nullptr;

		Instance_Buffer();

		void init(Ring_Buffer* global_staging_buffer, uint32_t initialize_buffer_size, uint32_t instance_size);

		uint32_t add_data(void* data);

		uint32_t get_index_by(uint32_t id);

		void update_data(uint32_t id, void* update_data);

		void remove_data(uint32_t id);

		void flush_data();

		void destroy() const;
	};

} // namespace Vulkan