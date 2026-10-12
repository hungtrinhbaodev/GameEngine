#pragma once
#include <id_generator.h>
#include <map>
#include <queue>
#include <sparse_set.h>
#include <vulkan/vk_buffer.h>
#include <vulkan/vk_common.h>
#include <vulkan/vulkan.h>

namespace Vulkan {

	struct SSBO_Buffer {

		struct Transfer_Data_Information {
			uint32_t dst_offset = 0;
			uint32_t src_offset = 0;
			uint32_t size_transfer = 0;
			VkBuffer dst_buffer = 0;
		};

		VkDevice device = VK_NULL_HANDLE;

		Buffer inner_buffer{};

		std::priority_queue<Buffer_Range, std::vector<Buffer_Range>, Buffer_Range_Compare> available_ranges;

		std::vector<Transfer_Data_Information> queue_transfer{};

		uint32_t current_offset = 0;

		Id_Generator range_id_generator{};

		Sparse_Set<Buffer_Range> ranges_by_id;

		void init(uint32_t initialize_size, VkPhysicalDevice physical_device, VkDevice device);

		uint32_t upload_data(void* data, uint32_t size);

		void update_data(uint32_t id, void* data, uint32_t offset = 0, uint32_t size = 0);

		void remove_data(uint32_t id);

		void update_dst_buffer_transfer(VkBuffer from, VkBuffer to);

		Buffer_Range view_slot(uint32_t id);

		void transfer_data_to(uint32_t id, VkBuffer dst_buffer, uint32_t dst_offset);

		std::map<VkBuffer, std::vector<VkBufferCopy>> make_copied_data();

		void flush_transfer_data();

		void record_flush_transfer_data(VkCommandBuffer command_buffer);

		void clear();

		void destroy();
	};

} // namespace Vulkan