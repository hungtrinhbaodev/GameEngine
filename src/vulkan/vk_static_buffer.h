#pragma once
#include <vulkan/vulkan.h>

#include <vulkan/vk_buffer.h>
#include <vulkan/vk_ring_buffer.h>

#include <vector>
#include <stack>
#include <map>
#include <queue>

namespace Vulkan {

	struct Static_Buffer_Range {

		uint32_t offset;
		
		uint32_t size;

		uint32_t using_size;

	};

	struct Static_Buffer_Range_Compare {
		inline bool operator() (const Static_Buffer_Range& a, const Static_Buffer_Range& b) const { return a.size > b.size; }
	};

	/*
		Using to storage all static data upload once use many 
		like vertex, mesh, indices,... of model
	*/
	struct Static_Buffer {

		Buffer inner_buffer;

		std::stack<int> available_ids;

		std::priority_queue<Static_Buffer_Range, std::vector<Static_Buffer_Range>, Static_Buffer_Range_Compare> available_ranges;

		std::map<uint32_t, Static_Buffer_Range> ranges_by_id;

		uint32_t available_size;

		uint32_t id_counter;

		uint32_t current_offset;

		Ring_Buffer* staging_buffer;

		void init(Ring_Buffer* global_staging_buffer, uint32_t initialize_size);

		uint32_t upload_data(uint32_t size, void* data);

		bool remove_data(uint32_t id);

		void destroy();

	};

}