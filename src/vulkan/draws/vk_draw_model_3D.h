#pragma once
#include <vulkan/draws/draw_structs.h>
#include <vulkan/vk_draw.h>

namespace Vulkan {

	namespace Model_3D {

		bool is_material_equal(uint32_t a, uint32_t b);

		void init();

		void destroy();

	} // namespace Model_3D

} // namespace Vulkan