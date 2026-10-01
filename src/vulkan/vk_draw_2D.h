#pragma once
#include <vulkan/vk_consts.h>
#include <vulkan/vulkan.h>

namespace Vulkan {

	namespace Draw_2D {

		struct Draw_2D_Information {
			Const::DRAW_ID draw_id = Const::DRAW_ID::UNDEFINED;
			/**
			 * @Note: unique material need to draw
			 * this object per draw id.
			 */
			uint32_t draw_material_id = 0;
			/**
			 * @Note: instance id of object draw
			 * in SSBO instance buffer.
			 */
			uint32_t instance_id = 0;
			/**
			 * @Note: index of object draw.
			 */
			uint32_t draw_index = -1;
			/**
			 * @Note: turn on/off object to draw.
			 */
			bool visible = false;
		};

		void init();

		void draw_2D();

	} // namespace Draw_2D

} // namespace Vulkan