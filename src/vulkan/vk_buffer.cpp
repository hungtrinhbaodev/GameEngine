#include <algorithm>

#include <vulkan/vk_core.h>
#include <vulkan/vk_buffer.h>
#include <vulkan/vk_utils.h>
#include <vulkan/vk_command_pool.h>
#include <vulkan/vk_queues.h>
#include <vulkan/vk_fences.h>
#include <vulkan/vk_structs.h>
#include <log.h>

namespace Vulkan {

	Buffer::Buffer() {}

	Buffer::Buffer(const Buffer& other) {
		size = other.size;
		usage_flags = other.usage_flags;
		property_flags = other.property_flags;
		device = other.device;
		physical_device = other.physical_device;
		buffer = other.buffer;
		memory = other.memory;
	};

	void Buffer::make_buffer(uint32_t size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags property_flags,
							 VkPhysicalDevice physical_device, VkDevice device) {

		if (device == VK_NULL_HANDLE) {
			device = Vulkan::device;
		}

		if (physical_device == VK_NULL_HANDLE) {
			physical_device = Vulkan::physical_device;
		}

		if (device == VK_NULL_HANDLE || physical_device == VK_NULL_HANDLE) {
			throw std::runtime_error("Vulkan fail to make buffer: try to init device and physical device first!");
		}

		this->usage_flags = usage_flags | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		this->property_flags = property_flags;
		this->size = size;
		this->device = device;
		this->physical_device = physical_device;

		VkBufferCreateInfo buffer_info{};
		buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_info.usage = this->usage_flags;
		buffer_info.size = static_cast<VkDeviceSize>(size);
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		Utils::vk_check_result(vkCreateBuffer(device, &buffer_info, nullptr, &buffer), "",
							   "Vulkan fail to create buffer!");

		VkMemoryRequirements memory_requirement;
		vkGetBufferMemoryRequirements(device, buffer, &memory_requirement);

		VkMemoryAllocateInfo allocate_info{};
		allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocate_info.allocationSize = static_cast<VkDeviceSize>(size);
		allocate_info.memoryTypeIndex =
			Utils::find_suitable_memory_type(memory_requirement.memoryTypeBits, property_flags, physical_device);

		Utils::vk_check_result(vkAllocateMemory(device, &allocate_info, nullptr, &memory), "",
							   "Vulkan fail to allocate buffer's memory!");

		vkBindBufferMemory(device, buffer, memory, 0);
	}

	void Buffer::copy_data(uint32_t size, void* data, uint32_t offset) const {
		if (is_host_visible_buffer()) {
			void* map_memory = nullptr;
			vkMapMemory(device, memory, offset, size, 0, &map_memory);
			memcpy(map_memory, data, static_cast<size_t>(size));
			vkUnmapMemory(device, memory);
		} else {
			Buffer staging{};
			staging.make_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
								VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			staging.copy_data(size, data, offset);
			Buffer::copy_buffer(staging, *this, 0, 0, size);
			staging.destroy();
		}
	}

	bool Buffer::is_host_visible_buffer() const {
		return property_flags & (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	}

	void Buffer::resize(uint32_t new_size) {

		Buffer new_buffer{};

		new_buffer.make_buffer(new_size, usage_flags | VK_BUFFER_USAGE_TRANSFER_DST_BIT, property_flags,
							   physical_device, device);

		uint32_t copy_size = std::min(size, new_size);
		Buffer::copy_buffer(*this, new_buffer, 0, 0, size);
		destroy();

		size = new_size;
		buffer = new_buffer.buffer;
		memory = new_buffer.memory;
	}

	void Buffer::destroy() const {
		vkDestroyBuffer(device, buffer, nullptr);
		vkFreeMemory(device, memory, nullptr);
	}

	void Buffer::copy_buffer(Buffer src_buffer, Buffer dst_buffer, uint32_t src_offset, uint32_t dst_offset,
							 uint32_t size) {
		if (src_buffer.buffer == VK_NULL_HANDLE || dst_buffer.buffer == VK_NULL_HANDLE) {
			throw std::runtime_error("Vulkan fail to copy buffer: try to make buffer first!");
		}

		if (src_offset + size > src_buffer.size) {
			throw std::runtime_error("Vulkan fail to copy buffer: make sure source buffer is enough size!");
		}

		if (dst_offset + size > dst_buffer.size) {
			throw std::runtime_error("Vulkan fail to copy buffer: make sure destination buffer is enough size!");
		}

		if (src_buffer.is_host_visible_buffer() && dst_buffer.is_host_visible_buffer()) {
			void* map_data;
			vkMapMemory(src_buffer.device, src_buffer.memory, src_offset, size, 0, &map_data);
			dst_buffer.copy_data(size, map_data, dst_offset);
			vkUnmapMemory(src_buffer.device, src_buffer.memory);
		} else {
			VkBufferCopy region{0, 0, size};
			_global_thread_pool
				->enqueue(
					[](Buffer& src, Buffer& dst, VkBufferCopy& region) {
						std::thread::id thread_id = std::this_thread::get_id();
						VkCommandBuffer command_buffer = API::request_command_buffer();
						VkFence fence = API::request_fence();

						VkCommandBufferBeginInfo begin_info = Structs::make_command_begin_info();

						vkBeginCommandBuffer(command_buffer, &begin_info);
						vkCmdCopyBuffer(command_buffer, src.buffer, dst.buffer, 1, &region);
						vkEndCommandBuffer(command_buffer);

						VkSubmitInfo submit_info = Structs::make_submit_info(&command_buffer);

						API::submit(submit_info, fence);
						API::on_fence_success(
							fence,
							[](VkCommandBuffer command_buffer, std::thread::id thread_id, VkFence fence) {
								API::release_command_buffer(command_buffer, thread_id);
								API::release_fence(fence);
							},
							command_buffer, thread_id, fence)
							.get();
					},
					src_buffer, dst_buffer, region)
				.get();
		}
	}

	std::vector<char> Buffer::parse_buffer(int offset, int parse_size) const {
#if _DEBUG
		if (offset >= size) {
			offset = size - 1;
		}
		if (parse_size < 0) {
			parse_size = size;
		}
		if (offset + parse_size > size) {
			throw std::runtime_error("Fail to parse buffer size parse out abound!");
		}
		std::vector<char> data;
		if (is_host_visible_buffer()) {
			void* map = nullptr;
			vkMapMemory(device, memory, 0, size, 0, &map);
			for (uint32_t i = offset; i < offset + parse_size; i++) {
				char* cur_char = (char*)map + i;
				data.push_back(*cur_char);
			}
			vkUnmapMemory(device, memory);
			Log::log_info("buffer data: ", data);
		} else {
			Buffer host_visible_buffer{};
			host_visible_buffer.make_buffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
											VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			Buffer::copy_buffer(*this, host_visible_buffer, 0, 0, size);
			auto parse_data = host_visible_buffer.parse_buffer(offset, parse_size);
			host_visible_buffer.destroy();
			return parse_data;
		}

		return data;
#endif
		return {};
	}

} // namespace Vulkan