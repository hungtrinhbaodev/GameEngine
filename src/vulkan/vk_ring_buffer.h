#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include <vulkan/vk_buffer.h>

namespace Vulkan {

	struct Ring_Buffer_Allocate_Info {

		int offset_src;

		int offset_dst;

		int size;
		  
		VkBuffer dst_buffer;
	};
	
	/*
		This buffer make a role like a global staging buffer
		can be storage data in phase staging 
	*/
	struct Ring_Buffer {

		int max_frame;

		int current_frame;

		std::vector<Buffer> inner_buffers;

		std::vector<int> current_frame_offsets;

		/*
			Real size of buffer will be max_size * current_frame_size
			current frame size will be change if buffer is out abound
		*/
		std::vector<uint32_t> max_frame_sizes;

		std::vector<Ring_Buffer_Allocate_Info> queue_upload_transfer;

		void init(int max_frame, uint32_t initialize_size);

		void start_frame(int current_fame);

		void upload_data(VkBuffer dst_buffer, uint32_t dst_offset, uint32_t size, void* data);

		void flush_frame();

		void destroy();
	};

}