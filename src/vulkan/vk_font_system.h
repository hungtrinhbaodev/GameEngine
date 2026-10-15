#pragma once
#include <geometry_structs.h>
#include <id_generator.h>
#include <map>
#include <sparse_set.h>
#include <vulkan/vk_common.h>
#include <vulkan/vk_ssbo_buffer.h>
#include <vulkan/vk_texture_system.h>

namespace Vulkan {

	struct Font_System {

		Id_Generator font_id_generator{};

		Texture_System texture_system{};

		Sparse_Set<Font> fonts{};

		SSBO_Buffer* ssbo = nullptr;

		std::unordered_map<std::string, uint32_t> file_to_ids{};

		void init(
			SSBO_Buffer* ssbo, VkDevice device, std::vector<VkDescriptorPool> descriptor_pools,
			std::array<VkDescriptorPool, Const::MAX_FRAMES_IN_FLIGHT> descriptor_pools_2,
			VkPhysicalDevice physical_device
		);

		uint32_t load_font(std::string font_path);

		const Font& view_font(uint32_t font_id);

		void destroy();
	};

} // namespace Vulkan