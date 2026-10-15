#pragma once
#include <vulkan/draws/draw_structs.h>
#include <vulkan/vk_common.h>
#include <vulkan/vk_consts.h>
#include <vulkan/vk_ssbo_buffer.h>
#include <vulkan/vk_static_buffer.h>
#include <vulkan/vulkan.h>

namespace Vulkan {
	/**
	 * @Note: using to debug.
	 */
	extern long long frame_count;

	void init_draw();

	void setup_draw();

	void sort_draws();

	void batching_draw_groups_2(VkCommandBuffer command_buffer, uint32_t frame_index);

	void draw(VkCommandBuffer command_buffer);

	void destroy_draw();

	Buffer& get_instance_buffer(uint32_t frame_index);

	const VkDescriptorSetLayout& get_layout(Const::DESCRITPOR_LAYOUT type);

	uint32_t load_texture(std::string texture_path);

	const Texture_View& view_texture(uint32_t texture_id);

	uint32_t load_font(std::string font_path);

	const Font& view_font(uint32_t font_id);

	const Texture_View& view_font_texture(uint32_t font_id);

	uint32_t load_model(std::string path);

	const Model_Information& view_model(uint32_t model_id);

	uint32_t get_primitive_transform_offset(
		uint32_t model_id, uint32_t scene_index, uint32_t mesh_index, uint32_t primitive_index
	);

	VkDescriptorSet get_storage_descriptor_set(uint32_t frame_index);

	/**
	 * @Note: index 0 for texture individual, index 1 for texture bucket.
	 */
	std::array<VkDescriptorSet, 2> get_descriptor_set_texture_sampler(uint32_t texture_id, uint32_t frame_index);

	/**
	 * @Note: index 0 for texture individual, index 1 for texture bucket.
	 */
	std::array<VkDescriptorSet, 2> get_descriptor_set_font_sampler(uint32_t font_id, uint32_t frame_index);

	void bind_draw_resource(
		VkCommandBuffer command_buffer, VkPipeline pipeline, VkPipelineLayout pipline_layout, VkBuffer indices_buffer,
		uint32_t indices_offset, VkIndexType index_type, uint32_t number_vertex_buffer,
		VkBuffer* binding_vertex_buffers, VkDeviceSize* vertex_buffer_offsets, uint32_t number_descriptor,
		VkDescriptorSet* binding_descriptor_sets
	);

	uint32_t make_rectange(const Draw_Attribute& draw_attributes, Rectangle_Attributes rectangle_attributes);

	void update_rectangle(uint32_t id, Rectangle_Attributes rectangle_attributes);

	uint32_t make_texture_2D(const Draw_Attribute& draw_attributes, const Texture_2D_Attributes& texture_attributes);

	uint32_t make_texture_2D_2(const Draw_Attribute& draw_attributes, const Texture_2D_Attributes& texture_attributes);

	void update_texture_2D(uint32_t id, const Texture_2D_Attributes& texture_attributes);

	uint32_t make_triangle(const Draw_Attribute& draw_attributes, const Triangle_Attribultes& triangle_attributes);

	void update_triangle(uint32_t id, const Triangle_Attribultes& triangle_attributes);

	uint32_t make_font_2D(const Draw_Attribute& draw_attributes, const Font_2D_Attributes& font_attributes);

	uint32_t make_model_3D(const Draw_Attribute& draw_attributes, const Model_3D_Attributes& model_attributes);

	void update_draw(uint32_t id, Draw_Attribute draw_attributes);

} // namespace Vulkan