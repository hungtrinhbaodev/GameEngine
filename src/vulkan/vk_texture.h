#pragma once

#include <string>
#include <vulkan/vk_image.h>
#include <vulkan/vulkan.h>

namespace Vulkan {

	struct Texture {

		VkDevice device = VK_NULL_HANDLE;

		uint32_t width = 0;

		uint32_t height = 0;

		uint32_t mip_level = 0;

		Image inner_image{};

		bool can_gpu_blit_image = false;

		void init(
			uint32_t width, uint32_t height, bool can_gpu_blit_image, VkPhysicalDevice physical_device, VkDevice device,
			VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, uint32_t mip_level = 1
		);

		void upload_data(void* data);

		void destroy() const;
	};

} // namespace Vulkan