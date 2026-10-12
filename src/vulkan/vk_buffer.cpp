#include <algorithm>

#include <log.h>
#include <vulkan/vk_buffer.h>
#include <vulkan/vk_structs.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

	Buffer::Buffer() {}

	Buffer::Buffer(const Buffer& other)
		: Buffer() {
		size = other.size;
		usage_flags = other.usage_flags;
		property_flags = other.property_flags;
		device = other.device;
		physical_device = other.physical_device;
		buffer = other.buffer;
		memory = other.memory;
		descriptor = other.descriptor;
	};

	Buffer::~Buffer() {}

	void Buffer::make_buffer(
		uint32_t size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags property_flags,
		VkPhysicalDevice physical_device, VkDevice device, bool auto_map
	) {
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

		Utils::vk_check_result(
			vkCreateBuffer(device, &buffer_info, nullptr, &buffer), "", "Vulkan fail to create buffer!"
		);

		VkMemoryRequirements memory_requirement;
		vkGetBufferMemoryRequirements(device, buffer, &memory_requirement);

		VkMemoryAllocateInfo allocate_info{};
		allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocate_info.allocationSize = static_cast<VkDeviceSize>(size);
		allocate_info.memoryTypeIndex =
			Utils::find_suitable_memory_type(memory_requirement.memoryTypeBits, property_flags, physical_device);

		Utils::vk_check_result(
			vkAllocateMemory(device, &allocate_info, nullptr, &memory), "", "Vulkan fail to allocate buffer's memory!"
		);

		vkBindBufferMemory(device, buffer, memory, 0);
		update_descriptor();

		if (auto_mapped && is_host_visible_buffer()) {
			this->auto_mapped = auto_map;
			vkMapMemory(device, memory, 0, size, 0, &mapped);
		}
	}

	void Buffer::copy_data(uint32_t size, void* data, uint32_t offset) const {
		if (is_host_visible_buffer()) {
			void* map_memory = nullptr;
			if (!auto_mapped) {
				vkMapMemory(device, memory, offset, size, 0, &map_memory);
			} else {
				map_memory = (char*)mapped + offset;
			}
			memcpy(map_memory, data, static_cast<size_t>(size));
			if (!auto_mapped) {
				vkUnmapMemory(device, memory);
			}
		} else {
			Buffer staging{};
			staging.make_buffer(
				size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, physical_device, device
			);
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

		new_buffer.make_buffer(
			new_size, usage_flags | VK_BUFFER_USAGE_TRANSFER_DST_BIT, property_flags, physical_device, device
		);

		uint32_t copied_size = std::min(size, new_size);
		Buffer::copy_buffer(*this, new_buffer, 0, 0, copied_size);
		destroy();

		size = new_size;
		buffer = new_buffer.buffer;
		memory = new_buffer.memory;
		update_descriptor();
	}

	Buffer Buffer::record_resize(VkCommandBuffer command_buffer, uint32_t new_size) {
		Buffer current{*this};
		Buffer new_buffer{};
		new_buffer.make_buffer(
			new_size, usage_flags | VK_BUFFER_USAGE_TRANSFER_DST_BIT, property_flags, physical_device, device
		);
		uint32_t copy_size = std::min(size, new_size);
		VkBufferMemoryBarrier copy_data_barrier =
			Structs::make_buffer_barier(0, VK_ACCESS_TRANSFER_WRITE_BIT, new_buffer.buffer, 0, copy_size);
		vkCmdPipelineBarrier(
			command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1,
			&copy_data_barrier, 0, nullptr
		);
		record_copy_buffer(command_buffer, *this, new_buffer, 0, 0, copy_size);
		size = new_size;
		buffer = new_buffer.buffer;
		memory = new_buffer.memory;
		update_descriptor();
		return current;
	}

	void Buffer::update_descriptor() {
		descriptor.buffer = buffer;
		descriptor.offset = 0;
		descriptor.range = size;
	}

	void Buffer::destroy() const {
		if (auto_mapped) {
			vkUnmapMemory(device, memory);
		}
		vkDestroyBuffer(device, buffer, nullptr);
		vkFreeMemory(device, memory, nullptr);
	}

	void Buffer::copy_buffer(
		Buffer src_buffer, Buffer dst_buffer, uint32_t src_offset, uint32_t dst_offset, uint32_t size
	) {
		if (src_buffer.buffer == VK_NULL_HANDLE || dst_buffer.buffer == VK_NULL_HANDLE) {
			throw std::runtime_error("Vulkan fail to copy buffer: try to make buffer first!");
		}

		if (src_buffer.device == VK_NULL_HANDLE || dst_buffer.device == VK_NULL_HANDLE ||
			src_buffer.device != dst_buffer.device) {
			throw std::runtime_error("Vulkan fail to copy buffer: device invalid!");
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
			VkBufferCopy region{src_offset, dst_offset, size};
			VkCommandBuffer command_buffer = Utils::start_commands(src_buffer.device);
			{
				record_copy_buffer(command_buffer, src_buffer, dst_buffer, src_offset, dst_offset, size);
			}
			Utils::finish_commands(src_buffer.device, command_buffer);
		}
	}

	void Buffer::record_copy_buffer(
		VkCommandBuffer command_buffer, Buffer src_buffer, Buffer dst_buffer, uint32_t src_offset, uint32_t dst_offset,
		uint32_t size
	) {
		if (src_buffer.buffer == VK_NULL_HANDLE || dst_buffer.buffer == VK_NULL_HANDLE) {
			throw std::runtime_error("Vulkan fail to copy buffer: try to make buffer first!");
		}

		if (src_buffer.device == VK_NULL_HANDLE || dst_buffer.device == VK_NULL_HANDLE ||
			src_buffer.device != dst_buffer.device) {
			throw std::runtime_error("Vulkan fail to copy buffer: device invalid!");
		}

		if (src_offset + size > src_buffer.size) {
			throw std::runtime_error("Vulkan fail to copy buffer: make sure source buffer is enough size!");
		}

		if (dst_offset + size > dst_buffer.size) {
			throw std::runtime_error("Vulkan fail to copy buffer: make sure destination buffer is enough size!");
		}
		VkBufferCopy region{src_offset, dst_offset, size};
		vkCmdCopyBuffer(command_buffer, src_buffer.buffer, dst_buffer.buffer, 1, &region);
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
		} else {
			Buffer host_visible_buffer{};
			host_visible_buffer.make_buffer(
				size, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, physical_device, device
			);
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