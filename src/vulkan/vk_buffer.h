#pragma once
#include <vulkan/vulkan.h>

#include <vector>

namespace Vulkan {

	struct Buffer {

		VkDevice device = VK_NULL_HANDLE;

		VkPhysicalDevice physical_device = VK_NULL_HANDLE;

		VkBuffer buffer = VK_NULL_HANDLE;

		VkDeviceMemory memory = VK_NULL_HANDLE;

		uint32_t size = 0;

		VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VkMemoryPropertyFlags property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		Buffer();

		Buffer(const Buffer& other);

		void make_buffer(uint32_t size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags property_flags,
						 VkPhysicalDevice physical_device = VK_NULL_HANDLE, VkDevice device = VK_NULL_HANDLE);

		void copy_data(uint32_t size, void* data, uint32_t offset = 0) const;

		void resize(uint32_t new_size);

		void destroy() const;

		bool is_host_visible_buffer() const;

		std::vector<char> parse_buffer(int offset = 0, int parse_size = -1) const;

		static void copy_buffer(Buffer src_buffer, Buffer dst_buffer, uint32_t src_offset, uint32_t dst_offset,
								uint32_t size);
	};

} // namespace Vulkan