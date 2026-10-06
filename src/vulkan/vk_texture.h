#pragma once

#include <string>
#include <vulkan/vk_image.h>
#include <vulkan/vulkan.h>

namespace Vulkan {

	struct Texture {

		uint32_t width = 0;

		uint32_t height = 0;

		Image inner_image{};

		void init(uint32_t width, uint32_t height, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB);

		void upload_data(void* data);

		void destroy() const;
	};

} // namespace Vulkan