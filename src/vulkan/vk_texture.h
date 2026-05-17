#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vk_image.h>
#include <string>

namespace Vulkan {

	struct Texture {

		uint32_t width = 0;

		uint32_t height = 0;

		Image inner_image{};

		void init(uint32_t width, uint32_t height);

		void upload_data(void* data);

		void destroy() const;
	};

}