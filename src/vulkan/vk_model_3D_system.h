#pragma once
#include <map>
#include <stack>
#include <string>
#include <vulkan/vk_common.h>
#include <vulkan/vk_static_buffer.h>
#include <vulkan/vk_texture_system.h>

namespace Vulkan {

	struct Model_3D_System {

		SSBO_Buffer* ssbo = nullptr;

		Static_Buffer* static_buffer = nullptr;

		Texture_System* texture_system = nullptr;

		std::stack<uint32_t> available_ids;

		uint32_t id_counter = 0;

		std::map<uint32_t, std::string> ids_to_files;

		std::map<std::string, uint32_t> files_to_ids;

		std::map<uint32_t, Model_Information> models;

		void init(Texture_System* texture_system, Static_Buffer* static_buffer, SSBO_Buffer* ssbo);

		uint32_t load_model(std::string path);

		const Model_Information& view_model(uint32_t model_id);

		void remove_model(uint32_t model_id);
	};

} // namespace Vulkan