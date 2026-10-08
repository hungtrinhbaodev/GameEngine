#include <stdexcept>
#include <vector>
#include <vulkan/vk_buffer.h>
#include <vulkan/vk_texture.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

	void Texture::init(
		uint32_t width, uint32_t height, bool can_gpu_blit_image, VkPhysicalDevice physical_device, VkDevice device,
		VkFormat format, uint32_t mip_level
	) {
		this->width = width;
		this->height = height;
		this->mip_level = mip_level;
		this->can_gpu_blit_image = can_gpu_blit_image;
		VkImageUsageFlags image_usages = (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT) |
										 (can_gpu_blit_image ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0);
		inner_image.make_image(
			width, height, format, VK_IMAGE_TILING_OPTIMAL, image_usages, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT, physical_device, device, 1, VK_IMAGE_VIEW_TYPE_2D, mip_level
		);
	}

	void Texture::upload_data(void* data) {
		Buffer staging_buffer{};
		std::vector<Buffer> mip_staging_buffers{};
		VkCommandBuffer command_buffer = Utils::start_commands();
		{
			inner_image.record_transition_image_layout(
				command_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 1, 0, mip_level
			);
			staging_buffer = inner_image.record_copy_image_data(command_buffer, width, height, data);
			if (mip_level > 1) {
				mip_staging_buffers = inner_image.record_generate_mipmap(command_buffer, data, can_gpu_blit_image, 0);
			}
			if (!can_gpu_blit_image || mip_level <= 1) {
				inner_image.record_transition_image_layout(
					command_buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0,
					1, 0, mip_level
				);
			}
		}
		Utils::finish_commands(command_buffer);
		staging_buffer.destroy();
		for (Buffer buffer : mip_staging_buffers) {
			buffer.destroy();
		}
		inner_image.update_descriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void Texture::destroy() const {
		inner_image.destroy();
	}

} // namespace Vulkan