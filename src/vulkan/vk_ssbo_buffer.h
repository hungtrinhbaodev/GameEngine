#pragma once
#include <id_generator.h>
#include <map>
#include <queue>
#include <sparse_set.h>
#include <vulkan/vk_buffer.h>
#include <vulkan/vulkan.h>

namespace Vulkan {

	struct SSBO_Buffer_Range {
		uint32_t offset = 0;
		uint32_t size = 0;
	};

	struct SSBO_Buffer_Range_Compare {
		inline bool operator()(const SSBO_Buffer_Range& a, const SSBO_Buffer_Range& b) const { return a.size > b.size; }
	};

	struct SSBO_Buffer {

		struct Transfer_Data_Information {
			uint32_t dst_offset = 0;
			uint32_t src_offset = 0;
			uint32_t size_transfer = 0;
			VkBuffer dst_buffer = 0;
		};

		Buffer dst_instance_buffer{};

		Buffer inner_buffer{};

		std::priority_queue<SSBO_Buffer_Range, std::vector<SSBO_Buffer_Range>, SSBO_Buffer_Range_Compare>
			available_ranges;

		std::vector<Transfer_Data_Information> queue_transfer{};

		uint32_t current_offset = 0;

		Sparse_Set<SSBO_Buffer_Range> ranges_by_id;

		void init(uint32_t initialize_size);

		uint32_t upload_data(void* data, uint32_t size);

		void update_data(uint32_t id, void* data, uint32_t offset = 0, uint32_t size = 0);

		void remove_data(uint32_t id);

		void update_dst_buffer_transfer(VkBuffer from, VkBuffer to);

		SSBO_Buffer_Range view_slot(uint32_t id);

		void transfer_data_to(uint32_t id, VkBuffer dst_buffer, uint32_t dst_offset);

		void flush_transfer_data();

		void destroy();
	};

} // namespace Vulkan