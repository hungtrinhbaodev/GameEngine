#include <vulkan/vk_texture_array.h>
#include <stdexcept>

namespace Vulkan {

	void Texture_Array::init(uint32_t number_layer, uint32_t width, uint32_t height) {
		this->number_layer = number_layer;
		used_indices.resize(number_layer, false);
		inner_image.make_image(
			width,
			height,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			number_layer
		);

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
		try {
			inner_image.transition_image_layout(inner_image.layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layer_index);
				inner_image.copy_image_data(inner_image.width, inner_image.height, data, layer_index);
			inner_image.transition_image_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		catch (std::exception e) {
			throw std::runtime_error(std::string("Fail to upload texture data in texture array: ") + e.what());
		}
		used_indices[layer_index] = true;
	}

	void Texture_Array::remove_data(uint32_t layer_index) {
		used_indices[layer_index] = false;
	}

	void Texture_Array::destroy() const {
		inner_image.destroy();
	}

}