#pragma once

#include <vulkan/vulkan.h>

#include <vulkan/vk_image.h>

#include <vector>

namespace Vulkan {

	struct Texture_Array {

		uint32_t number_layer = 0;

		std::vector<bool> used_indices;
		
		Image inner_image;

		void init(uint32_t number_layer, uint32_t width, uint32_t height);

		int find_availale_slot() const;

		void upload_data(uint32_t layer_index, void* data);

		void remove_data(uint32_t layer_index);

		void destroy() const ;

	};

}