#pragma once

#include <vulkan/vulkan.h>

#include <vulkan/vk_image.h>

#include <vector>

namespace Vulkan {

	struct Texture_Array {

		uint32_t number_layer = 0;

		std::vector<bool> used_indices;

		Image inner_image;

		uint32_t mip_level = 1;

		bool can_gpu_blit_image = false;

		void init(
			uint32_t number_layer, uint32_t width, uint32_t height, bool can_gpu_blit_image,
			VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, uint32_t mip_level = 1
		);

		int find_availale_slot() const;

		void upload_data(uint32_t layer_index, void* data);

		void remove_data(uint32_t layer_index);

		void destroy() const;
	};

} // namespace Vulkan