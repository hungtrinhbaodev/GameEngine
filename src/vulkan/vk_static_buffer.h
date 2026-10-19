#pragma once
#include <vulkan/vulkan.h>

#include <id_generator.h>
#include <sparse_set.h>
#include <vulkan/vk_buffer.h>
#include <vulkan/vk_ssbo_buffer.h>

#include <iostream>
#include <map>
#include <queue>
#include <stack>
#include <vector>
#include <vulkan/vk_common.h>

namespace Vulkan {

	struct Static_Buffer {

		VkDevice device = VK_NULL_HANDLE;

		uint32_t current_frame = 0;

		Buffer inner_buffer{};

		Buffer staging_buffer{};

		Id_Generator range_id_generator{};

		Sparse_Set<Buffer_Range> id_to_ranges{};

		std::priority_queue<Buffer_Range, std::vector<Buffer_Range>, Buffer_Range_Compare> available_ranges;

		uint32_t current_offset = 0;

		uint32_t size_required = 0;

		std::vector<VkFence> draw_fences{};

		Resize_Buffer_Synchronize_Information resize_sync_info{};

		Copied_Data_Synchronize_Information copied_sync_info{};

		std::vector<uint32_t> range_removed_ids{};

		Sparse_Set<Resource_Synchronize_Information> queue_remove_range{};

		uint32_t current_staging_offset = 0;

		Sparse_Set<Buffer_Range> id_to_staging_ranges{};

		std::priority_queue<Buffer_Range, std::vector<Buffer_Range>, Buffer_Range_Compare> available_staging_ranges{};

		std::vector<VkBufferCopy> copied_data{};

		void init(
			uint32_t initialize_size, uint32_t initialize_staging_size, VkBufferUsageFlags usage_flags,
			const std::vector<VkFence>& draw_fences, VkPhysicalDevice physical_device, VkDevice device
		);

		/**
		 * @Note: using to check resource async
		 * like remove action need to be confirm
		 * all draw finish to release allocator!
		 */
		void wait_synchronize_finish(uint32_t frame_index);

		uint32_t upload_data(void* data, uint32_t size, uint32_t element_stride = 1);

		const Buffer_Range& view_slot(uint32_t id);

		bool remove_data(uint32_t id);

		void flush_removed_data(uint32_t frame_index);

		void flush_data(VkCommandBuffer command_buffer, uint32_t frame_index);

		void clear();

		void destroy();
	};

} // namespace Vulkan