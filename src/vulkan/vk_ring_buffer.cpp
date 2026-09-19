#include <vulkan/vk_ring_buffer.h>
#include <vulkan/vk_fences.h>
#include <vulkan/vk_utils.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_command_pool.h>
#include <vulkan/vk_queues.h>
#include <vulkan/vk_structs.h>
#include <map>
#include <log.h>

namespace Vulkan {

	void Ring_Buffer::init(int max_frame, uint32_t initialize_size) {
		this->max_frame = max_frame;
		for (int i = 0; i < this->max_frame; i++) {
			max_frame_sizes.push_back(initialize_size);
			Buffer inner_buffer{};
			inner_buffer.make_buffer(
				initialize_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
			);
			inner_buffers.push_back(inner_buffer);
			current_frame_offsets.push_back(0);
		}
	}

	void Ring_Buffer::start_frame(int current_frame) {
		this->current_frame = current_frame % max_frame;
		current_frame_offsets[this->current_frame] = 0;
	}

	void Ring_Buffer::upload_data(VkBuffer dst_buffer, uint32_t dst_offset, uint32_t size, void* data) {

		uint32_t max_frame_size = max_frame_sizes[current_frame];
		uint32_t current_frame_offset = current_frame_offsets[current_frame];

		// Check size off buffer is enough we resize buffer at this frame
		if (current_frame_offset + size > max_frame_size) {
			max_frame_size = (uint32_t)((current_frame_offset + size) * 1.5f);
			inner_buffers[current_frame].resize(max_frame_size);
			max_frame_sizes[current_frame] = max_frame_size;
		}

		// Add data to staging inner at frame buffer
		inner_buffers[current_frame].copy_data(size, data, current_frame_offset);

		// Update queue transfer that need to be flush
		Ring_Buffer_Allocate_Info allocate_info{current_frame_offset, dst_offset, size, dst_buffer};
		queue_upload_transfer.push_back(allocate_info);

		// Update current offset at this frame
		current_frame_offset += size;
		current_frame_offsets[current_frame] = current_frame_offset;
	}

	void Ring_Buffer::flush_frame() {

		Buffer& inner_buffer = inner_buffers[current_frame];
		std::map<VkBuffer, std::vector<VkBufferCopy>> copied_data;

		for (const Ring_Buffer_Allocate_Info& allocate_info : queue_upload_transfer) {
			VkBuffer dst_buffer = allocate_info.dst_buffer;
			if (copied_data.find(dst_buffer) == copied_data.end()) {
				copied_data[dst_buffer] = {};
			}
			VkBufferCopy region{
				static_cast<VkDeviceSize>(allocate_info.offset_src),
				static_cast<VkDeviceSize>(allocate_info.offset_dst), static_cast<VkDeviceSize>(allocate_info.size)
			};
			copied_data[dst_buffer].push_back(region);
		}

		if (copied_data.size() <= 0) {
			queue_upload_transfer.clear();
			return;
		}

		_global_thread_pool
			->enqueue(
				[](std::map<VkBuffer, std::vector<VkBufferCopy>>& copied_data, VkBuffer src_buffer) {
					std::thread::id thread_id = std::this_thread::get_id();
					VkCommandBuffer command_buffer = API::request_command_buffer();
					VkFence fence = API::request_fence();

					VkCommandBufferBeginInfo begin_info = Structs::make_command_begin_info();
					vkBeginCommandBuffer(command_buffer, &begin_info);
					for (auto& [dst_buffer, copied_ranges] : copied_data) {
						vkCmdCopyBuffer(
							command_buffer, src_buffer, dst_buffer, copied_ranges.size(), copied_ranges.data()
						);
					}
					vkEndCommandBuffer(command_buffer);
					VkSubmitInfo submit_info = Structs::make_submit_info(&command_buffer);

					API::submit(submit_info, fence);
					API::on_fence_success(
						fence,
						[](std::thread::id thread_id, VkCommandBuffer command_buffer, VkFence fence) {
							API::release_command_buffer(command_buffer, thread_id);
							API::release_fence(fence);
						},
						thread_id, command_buffer, fence
					)
						.get();
				},
				copied_data, inner_buffer.buffer
			)
			.get();

		queue_upload_transfer.clear();
	}

	void Ring_Buffer::destroy() {

		for (Buffer& buffer : inner_buffers) {
			buffer.destroy();
		}
	}

}; // namespace Vulkan