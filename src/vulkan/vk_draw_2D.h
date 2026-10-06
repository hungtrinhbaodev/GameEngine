#pragma once
#include <vulkan/draws/draw_structs.h>
#include <vulkan/vk_consts.h>
#include <vulkan/vk_ssbo_buffer.h>
#include <vulkan/vk_static_buffer.h>
#include <vulkan/vulkan.h>

namespace Vulkan {

	namespace Draw_2D {

		struct Draw_2D_Information {
			Const::DRAW_ID draw_type = Const::DRAW_ID::UNDEFINED;
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
			uint32_t draw_index = 0;
			/**
			 * @Note: turn on/off object to draw.
			 */
			bool visible = false;
			/**
			 * @Note: when object draw has same
			 * draw index the order draw will
			 * be sort by create_index
			 */
			uint32_t create_index = 0;
		};

		/**
		 * @Note: using to debug.
		 */
		extern long long frame_count;

		void init();

		void draw(VkCommandBuffer command_buffer);

		void destroy();

		SSBO_Buffer& get_ssbo();

		Static_Buffer& get_vertex_buffer(size_t vertex_size);

		Static_Buffer& get_indices_buffer(size_t indices_size);

		Buffer& get_instance_buffer();

		void bind_draw_resource(
			VkCommandBuffer command_buffer, VkPipeline pipeline, VkPipelineLayout pipline_layout,
			VkBuffer indices_buffer, uint32_t indices_offset, VkIndexType index_type, uint32_t number_vertex_buffer,
			VkBuffer* binding_vertex_buffers, VkDeviceSize* vertex_buffer_offsets, uint32_t number_descriptor,
			VkDescriptorSet* binding_descriptor_sets
		);

		uint32_t make_rectange(const Draw_2D_Attribute& draw_attributes, Rectangle_Attributes rectangle_attributes);

		void update_rectangle(uint32_t id, Rectangle_Attributes rectangle_attributes);

		uint32_t make_texture_2D(
			const Draw_2D_Attribute& draw_attributes, const Texture_2D_Attributes& texture_attributes
		);

		void update_texture_2D(uint32_t id, const Texture_2D_Attributes& texture_attributes);

		uint32_t make_triangle(
			const Draw_2D_Attribute& draw_attributes, const Triangle_Attribultes& triangle_attributes
		);

		void update_triangle(uint32_t id, const Triangle_Attribultes& triangle_attributes);

		uint32_t make_font_2D(const Draw_2D_Attribute& draw_attributes, const Font_2D_Attributes& font_attributes);

		void update_draw(uint32_t id, Draw_2D_Attribute draw_attributes);

	} // namespace Draw_2D

} // namespace Vulkan