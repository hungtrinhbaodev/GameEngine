#pragma once
#include <array>
#include <id_generator.h>
#include <map>
#include <queue>
#include <sparse_set.h>
#include <vulkan/vk_buffer.h>
#include <vulkan/vk_common.h>
#include <vulkan/vulkan.h>

namespace Vulkan {

	struct SSBO_Buffer {

		uint32_t current_frame = 0;

		std::array<Resize_Buffer_Synchronize_Information, Const::MAX_FRAMES_IN_FLIGHT> frame_resize_storage_buffer_info;

		std::array<Buffer, Const::MAX_FRAMES_IN_FLIGHT> storage_buffers;

		std::array<Sparse_Set<bool>, Const::MAX_FRAMES_IN_FLIGHT> frame_dirty_ids;

		VkDevice device = VK_NULL_HANDLE;

		std::array<Buffer, Const::MAX_FRAMES_IN_FLIGHT> staging_buffers;

		std::priority_queue<Buffer_Range, std::vector<Buffer_Range>, Buffer_Range_Compare> available_ranges;

		std::vector<uint32_t> range_remove_ids;

		uint32_t current_offset = 0;

		Id_Generator range_id_generator{};

		Sparse_Set<Buffer_Range> ranges_by_id;

		std::vector<VkBufferCopy> copy_ranges;

		std::vector<uint8_t> bytes_data;

		void init(uint32_t initialize_size, VkPhysicalDevice physical_device, VkDevice device);

		void setup_frame(uint32_t current_frame);

		uint32_t upload_data(const void* data, uint32_t size, uint32_t element_stride = 1);

		void update_data(uint32_t id, void* data, uint32_t offset = 0, uint32_t size = 0);

		void remove_data(uint32_t id);

		const Buffer_Range& view_slot(uint32_t id);

		void flush_data_to_staging_buffer(uint32_t current_frame);

		void flush_data_to_storage_buffer(VkCommandBuffer command_buffer, uint32_t current_frame);

		bool is_storage_buffer_resizing(uint32_t frame_index);

		void destroy();
	};

} // namespace Vulkan