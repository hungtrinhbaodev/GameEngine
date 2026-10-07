#include <future>

#include <stb_image_resize2.h>
#include <vulkan/vk_command_pool.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_fences.h>
#include <vulkan/vk_image.h>
#include <vulkan/vk_queues.h>
#include <vulkan/vk_structs.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

	Image::Image() {}

	Image::Image(const Image& other) {
		width = other.width;
		height = other.height;
		image = other.image;
		view = other.view;
		format = other.format;
		sampler = other.sampler;
		aspect_flags = other.aspect_flags;
		physical_device = other.physical_device;
		device = other.device;
		array_layers = other.array_layers;
		image_view_type = other.image_view_type;
		descriptor_image_layers = other.descriptor_image_layers;
		mip_level = other.mip_level;
	}

	void Image::make_image(
		uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties, VkImageAspectFlags aspect_flags, uint32_t array_layers,
		VkImageViewType image_view_type, uint32_t mip_level, VkPhysicalDevice physical_device, VkDevice device
	) {

		if (device == VK_NULL_HANDLE) {
			device = Vulkan::device;
		}

		if (physical_device == VK_NULL_HANDLE) {
			physical_device = Vulkan::physical_device;
		}

		if (device == VK_NULL_HANDLE || physical_device == VK_NULL_HANDLE) {
			throw std::runtime_error("Vulkan fail to make image: try to init device and physical device first!");
		}

		this->device = device;
		this->physical_device = physical_device;
		this->aspect_flags = aspect_flags;
		this->format = format;
		this->width = width;
		this->height = height;
		this->array_layers = array_layers;
		this->image_view_type = image_view_type;
		this->mip_level = mip_level;
		this->descriptor_image_layers.resize(array_layers, {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED});

		VkImageCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		create_info.imageType = VK_IMAGE_TYPE_2D;
		create_info.extent.width = width;
		create_info.extent.height = height;
		create_info.extent.depth = 1;
		create_info.arrayLayers = array_layers;
		create_info.format = format;
		create_info.tiling = tiling;
		create_info.mipLevels = mip_level;
		create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		create_info.usage = usage;
		create_info.samples = VK_SAMPLE_COUNT_1_BIT;
		create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateImage(device, &create_info, nullptr, &image) != VK_SUCCESS) {
			throw std::runtime_error("Fail to create image!");
		}

		VkMemoryRequirements memory_requirements{};
		vkGetImageMemoryRequirements(device, image, &memory_requirements);

		VkMemoryAllocateInfo allocate_info{};
		allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocate_info.allocationSize = memory_requirements.size;
		allocate_info.memoryTypeIndex =
			Utils::find_suitable_memory_type(memory_requirements.memoryTypeBits, properties, physical_device);

		Utils::vk_check_result(
			vkAllocateMemory(device, &allocate_info, nullptr, &memory), "", "Vulkan fail to allocate image memory!"
		);

		vkBindImageMemory(device, image, memory, 0);

		view = Utils::create_imageview_from_image(
			image, format, aspect_flags, device, this->array_layers, image_view_type
		);
		if (usage | VK_IMAGE_USAGE_SAMPLED_BIT) {
			make_sampler();
		}
	}

	void Image::record_transition_image_layout(
		VkCommandBuffer command_buffer, VkImageLayout old_layout, VkImageLayout new_layout, uint32_t base_layer,
		uint32_t number_layer, uint32_t base_mip_level, uint32_t number_mip_level
	) {

		VkImageMemoryBarrier barrier_info{};
		barrier_info.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier_info.oldLayout = old_layout;
		barrier_info.newLayout = new_layout;
		barrier_info.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier_info.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier_info.image = image;
		barrier_info.subresourceRange.aspectMask = aspect_flags;
		barrier_info.subresourceRange.baseArrayLayer = base_layer;
		barrier_info.subresourceRange.baseMipLevel = base_mip_level;
		barrier_info.subresourceRange.levelCount = number_mip_level;
		barrier_info.subresourceRange.layerCount = number_layer;

		VkPipelineStageFlags src_stage;
		VkPipelineStageFlags dst_stage;

		if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier_info.srcAccessMask = 0;
			barrier_info.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		} else if (
			old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		) {
			barrier_info.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier_info.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		} else if (
			old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		) {
			barrier_info.srcAccessMask = 0;
			barrier_info.dstAccessMask =
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		} else if (
			old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		) {
			barrier_info.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier_info.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			src_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		} else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier_info.srcAccessMask = 0;
			barrier_info.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		} else if (
			old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
		) {
			barrier_info.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier_info.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		} else if (
			old_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		) {
			barrier_info.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier_info.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		} else {
			throw std::runtime_error("Vulkan transfer layout are not supported!");
		}
		vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier_info);
	}

	void Image::transition_image_layout(VkImageLayout old_layout, VkImageLayout new_layout, uint32_t layer_index) {
		VkCommandBuffer command_buffer = Utils::start_commands();
		{
			record_transition_image_layout(command_buffer, old_layout, new_layout, layer_index);
		}
		Utils::finish_commands(command_buffer);
		update_descriptor(new_layout, layer_index);
	}

	std::vector<Buffer> Image::record_generate_mipmap(
		VkCommandBuffer command_buffer, void* data, bool can_gpu_blit_image, int layer_index
	) {
		/**
		 * Note: case blit enabled in GPU we use this scope.
		 */
		/**
		 * @Note: if not support we make an image by resize and copy it into GPU.
		 */
		std::vector<Buffer> staging_buffers{};
		if (can_gpu_blit_image) {
			int mip_width = width, mip_height = height;
			for (int i = 1; i < mip_level; i++) {
				int src_mip_level = i - 1;
				record_transition_image_layout(
					command_buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					layer_index, 1, src_mip_level, 1
				);
				{
					VkImageBlit blit = Structs::make_image_blit(mip_width, mip_height, src_mip_level, layer_index, 1);
					vkCmdBlitImage(
						command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
						VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR
					);
				}
				record_transition_image_layout(
					command_buffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					layer_index, 1, src_mip_level, 1
				);
				mip_width = mip_width > 1 ? mip_width / 2 : mip_width;
				mip_height = mip_height > 1 ? mip_height / 2 : mip_height;
			}
			record_transition_image_layout(
				command_buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				layer_index, 1, mip_level - 1, 1
			);
		} else {
			std::vector<std::future<Buffer>> tasks{};
			std::vector<uint32_t> mip_widths{};
			std::vector<uint32_t> mip_heights{};
			uint32_t mip_width = width;
			uint32_t mip_height = height;
			for (int i = 1; i < mip_level; i++) {
				mip_width = mip_width > 1 ? mip_width / 2 : mip_width;
				mip_height = mip_height > 1 ? mip_height / 2 : mip_height;
				mip_widths.push_back(mip_width);
				mip_heights.push_back(mip_height);
			}
			for (int i = 0; i < mip_widths.size(); i++) {
				uint32_t mip_width = mip_widths[i];
				uint32_t mip_height = mip_heights[i];
				auto blit =
					[this](uint32_t mip_width, uint32_t mip_height, void* data, VkCommandBuffer command_buffer) {
						int channels = Utils::get_number_channel_by(format);
						std::vector<uint8_t> pixels_mip(mip_width * mip_height * channels);
						void* result = nullptr;
						if (channels == 1) {
							result = stbir_resize_uint8_linear(
								(const unsigned char*)data, width, height, 0, pixels_mip.data(), mip_width, mip_height,
								0, STBIR_1CHANNEL
							);
						} else {
							result = stbir_resize(
								data, width, height, 0, pixels_mip.data(), mip_width, mip_height, 0, STBIR_RGBA,
								STBIR_TYPE_UINT8, STBIR_EDGE_CLAMP, STBIR_FILTER_DEFAULT
							);
						}
						Buffer staging{};
						if (result == nullptr) {
							throw std::runtime_error("Fail to resize texture to blit image!");
						}
						VkDeviceSize image_size = mip_width * mip_height * channels;
						staging.make_buffer(
							image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
							VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
						);
						staging.copy_data(image_size, pixels_mip.data());
						return staging;
					};
				auto task = _global_thread_pool->enqueue(blit, mip_width, mip_height, data, command_buffer);
				tasks.push_back(std::move(task));
			}
			for (int i = 0; i < mip_widths.size(); i++) {
				uint32_t mip_width = mip_widths[i];
				uint32_t mip_height = mip_heights[i];
				Buffer staging_buffer = tasks[i].get();
				if (staging_buffer.size <= 0) {
					throw std::runtime_error("Fail to resize texture to blit image!");
				}
				record_copy_image_data_with_buffer(
					command_buffer, mip_width, mip_height, staging_buffer, layer_index, i + 1
				);
				staging_buffers.push_back(staging_buffer);
			}
		}
		return staging_buffers;
	}

	void Image::record_copy_image_data_with_buffer(
		VkCommandBuffer command_buffer, uint32_t width_copy, uint32_t height_copy, Buffer staging_buffer,
		uint32_t layer_index, uint32_t mip_level_index
	) {
		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = mip_level_index;
		region.imageSubresource.baseArrayLayer = layer_index;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = {0, 0, 0};
		region.imageExtent = {(uint32_t)width_copy, (uint32_t)height_copy, 1};
		vkCmdCopyBufferToImage(
			command_buffer, staging_buffer.buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region
		);
	}

	Buffer Image::record_copy_image_data(
		VkCommandBuffer command_buffer, uint32_t width, uint32_t height, void* pixels, uint32_t layer_index,
		uint32_t mip_level_index
	) {
		if ((this->width != width || this->height != height) && mip_level_index == 0) {
			throw std::runtime_error("Vulkan fail to copy image data: wrong size image!");
		}
		VkDeviceSize image_size = width * height * Utils::get_number_channel_by(format);
		Buffer staging{};
		staging.make_buffer(
			image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);
		staging.copy_data(image_size, pixels);
		record_copy_image_data_with_buffer(command_buffer, width, height, staging, layer_index, mip_level_index);
		return staging;
	}

	void Image::copy_image_data(uint32_t width, uint32_t height, void* pixels, uint32_t layer_index) {
		Buffer staging{};
		VkCommandBuffer command_buffer = Utils::start_commands();
		{
			staging = record_copy_image_data(command_buffer, width, height, pixels, layer_index);
		}
		Utils::finish_commands(command_buffer);
		staging.destroy();
	}

	void Image::make_sampler() {

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(physical_device, &properties);

		VkSamplerCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		create_info.magFilter = VK_FILTER_LINEAR;
		create_info.minFilter = VK_FILTER_LINEAR;
		create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		create_info.anisotropyEnable = VK_FALSE;
		create_info.maxAnisotropy = 1.0f;
		create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		create_info.unnormalizedCoordinates = VK_FALSE;
		create_info.compareEnable = VK_FALSE;
		create_info.compareOp = VK_COMPARE_OP_ALWAYS;
		create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		create_info.minLod = 0.f;
		create_info.maxLod = VK_LOD_CLAMP_NONE;

		Utils::vk_check_result(
			vkCreateSampler(device, &create_info, nullptr, &sampler), "", "Vulkan fail to create image sampler!"
		);
	}

	void Image::update_descriptor(VkImageLayout image_layout, int layer_index) {
		layer_index = std::max(0, layer_index);
		VkDescriptorImageInfo& descriptor = descriptor_image_layers[layer_index];
		descriptor.imageLayout = image_layout;
		descriptor.imageView = view;
		descriptor.sampler = sampler;
	}

	VkDescriptorImageInfo& Image::get_descriptor_info(int layer_index) {
		layer_index = std::max(0, layer_index);
		return descriptor_image_layers[layer_index];
	}

	void Image::destroy() const {

		if (sampler != VK_NULL_HANDLE) {
			vkDestroySampler(device, sampler, nullptr);
		}

		if (view != VK_NULL_HANDLE) {
			vkDestroyImageView(device, view, nullptr);
		}

		if (memory != VK_NULL_HANDLE) {
			vkFreeMemory(device, memory, nullptr);
		}

		if (image != VK_NULL_HANDLE) {
			vkDestroyImage(device, image, nullptr);
		}
	}

} // namespace Vulkan