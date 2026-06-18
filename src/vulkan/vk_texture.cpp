#include <vulkan/vk_texture.h>
#include <vector>
#include <stdexcept>

namespace Vulkan {

	void Texture::init(uint32_t width, uint32_t height) {

		this->width = width;
		this->height = height;
		inner_image.make_image(width, height, VK_FORMAT_R8G8_SRGB, VK_IMAGE_TILING_OPTIMAL,
							   VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
							   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}

	void Texture::upload_data(void* data) {
		try {
			inner_image.transition_image_layout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			inner_image.copy_image_data(width, height, data);
			inner_image.transition_image_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
												VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		} catch (std::exception e) {
			throw std::runtime_error(std::string("Fail to upload texture data in texture: ") + e.what());
		}
	}

	void Texture::destroy() const {
		inner_image.destroy();
	}

} // namespace Vulkan