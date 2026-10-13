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

		struct Transfer_Data_Information {
			uint32_t dst_offset = 0;
			uint32_t src_offset = 0;
			uint32_t size_transfer = 0;
			VkBuffer dst_buffer = 0;
		};

		struct Remove_Buffer_Synchronize_Information {
			Buffer remove_buffer{};
			Resource_Synchronize_Information sync_info{};
		};

		uint32_t current_frame = 0;

		std::array<Resize_Buffer_Synchronize_Information, Const::MAX_FRAMES_IN_FLIGHT> frame_resize_info{};

		std::array<Buffer, Const::MAX_FRAMES_IN_FLIGHT> storage_buffers{};

		std::array<Sparse_Set<bool>, Const::MAX_FRAMES_IN_FLIGHT> frame_dirty_ids{};

		VkDevice device = VK_NULL_HANDLE;

		Buffer inner_buffer{};

		std::priority_queue<Buffer_Range, std::vector<Buffer_Range>, Buffer_Range_Compare> available_ranges;

		std::vector<Transfer_Data_Information> queue_transfer{};

		std::vector<Remove_Buffer_Synchronize_Information> queue_remove_buffer{};

		Sparse_Set<Resource_Synchronize_Information> queue_remove_range{};

		std::vector<uint32_t> range_remove_ids{};

		uint32_t current_offset = 0;

		Id_Generator range_id_generator{};

		Sparse_Set<Buffer_Range> ranges_by_id;

		void init(uint32_t initialize_size, VkPhysicalDevice physical_device, VkDevice device);

		void setup_frame(uint32_t current_frame);

		uint32_t upload_data(void* data, uint32_t size, uint32_t element_stride = 1);

		void update_data(uint32_t id, void* data, uint32_t offset = 0, uint32_t size = 0);

		void remove_data(uint32_t id);

		Buffer_Range view_slot(uint32_t id);

		void transfer_data_to(uint32_t id, VkBuffer dst_buffer, uint32_t dst_offset);

		void flush_data_to_storage_buffer(VkCommandBuffer command_buffer, uint32_t current_frame);

		std::map<VkBuffer, std::vector<VkBufferCopy>> make_copied_data();

		void flush_removed_data(uint32_t frame_index);

		void flush_transfer_data();

		void record_flush_transfer_data(VkCommandBuffer command_buffer);

		void destroy();
	};

} // namespace Vulkan