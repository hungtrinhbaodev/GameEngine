#pragma once
#include <vulkan/vk_consts.h>

namespace Vulkan {

	struct Draw_Info {
		Const::DRAW_ID draw_id;
		uint32_t texture_id;
		uint32_t slot_indices_id;
		uint32_t slot_vertices_id;
	};

} // namespace Vulkan