#pragma once
#include <consts.h>
#include <vulkan/draws/vk_draw_attributes.h>
#include <vulkan/vk_draw.h>

namespace Vulkan {

	namespace Draw_Clipping_2D {

		void init_clipping(Draw_Information& draw_info, const Clipping_2D& attributes);

		void update_clipping(Draw_Information& draw_info, const Clipping_2D& attributes);

		void remove_clipping(uint32_t unique_id);

		void debug(uint32_t frame_index);

		Clipping_Helper get_clipping_helper(const uint32_t& unique_id);

	} // namespace Draw_Clipping_2D

} // namespace Vulkan