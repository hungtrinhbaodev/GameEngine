#include <stdexcept>
#include <vulkan/vk_command_pool.h>
#include <vulkan/vk_fences.h>
#include <vulkan/vk_queues.h>
#include <vulkan/vk_structs.h>
#include <vulkan/vk_texture_array.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

	void Texture_Array::init(
		uint32_t number_layer, uint32_t width, uint32_t height, bool can_gpu_blit_image, VkFormat format,
		uint32_t mip_level
	) {
		this->number_layer = number_layer;
		this->mip_level = mip_level;
		this->can_gpu_blit_image = can_gpu_blit_image;
		used_indices.resize(number_layer, false);
		VkImageUsageFlags image_usages = (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT) |
										 (can_gpu_blit_image ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0);
		inner_image.make_image(
			width, height, format, VK_IMAGE_TILING_OPTIMAL, image_usages, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT, number_layer, VK_IMAGE_VIEW_TYPE_2D_ARRAY, mip_level
		);
		VkCommandBuffer command_buffer = Utils::start_commands();
		{
			inner_image.record_transition_image_layout(
				command_buffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, number_layer, 0,
				mip_level
			);
		}
		Utils::finish_commands(command_buffer);
		for (int i = 0; i < number_layer; i++) {
			inner_image.update_descriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, i);
		}
	}

	int Texture_Array::find_availale_slot() const {
		int available_index = -1;
		for (int i = 0; i < used_indices.size(); i++) {
			bool is_used = used_indices[i];
			if (!is_used) {
				available_index = i;
				break;
			}
		}
		return available_index;
	}

	void Texture_Array::upload_data(uint32_t layer_index, void* data) {
		if (used_indices[layer_index]) {
			throw std::runtime_error("Fail to upload data textrue in texture array: layer index upload is using");
		}
		Buffer staging_buffer{};
		std::vector<Buffer> mip_staging_buffers{};
		VkCommandBuffer command_buffer = Utils::start_commands();
		{
			inner_image.record_transition_image_layout(
				command_buffer, inner_image.get_descriptor_info(layer_index).imageLayout,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layer_index, 1, 0, mip_level
			);
			staging_buffer = inner_image.record_copy_image_data(
				command_buffer, inner_image.width, inner_image.height, data, layer_index
			);
			if (mip_level > 1) {
				mip_staging_buffers =
					inner_image.record_generate_mipmap(command_buffer, data, can_gpu_blit_image, layer_index);
			}
			if (!can_gpu_blit_image || mip_level <= 1) {
				inner_image.record_transition_image_layout(
					command_buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					layer_index, 1, 0, mip_level
				);
			}
		}
		Utils::finish_commands(command_buffer);
		staging_buffer.destroy();
		for (Buffer buffer : mip_staging_buffers) {
			buffer.destroy();
		}
		inner_image.update_descriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, layer_index);
		used_indices[layer_index] = true;
	}

	void Texture_Array::remove_data(uint32_t layer_index) {
		used_indices[layer_index] = false;
	}

	void Texture_Array::destroy() const {
		inner_image.destroy();
	}

} // namespace Vulkan