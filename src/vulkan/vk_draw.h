#pragma once
#include <handle.h>
#include <vulkan/draws/vk_draw_attributes.h>
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

	void setup_draw(float dt);

	void sort_draws();

	void batching_draw_groups(VkCommandBuffer command_buffer, uint32_t frame_index);

	void draw(VkCommandBuffer command_buffer);

	void destroy_draw();

	void show_debug();

	size_t get_instance_size(Const::DRAW_ID draw_type);

	Buffer& get_instance_buffer(uint32_t frame_index);

	const VkDescriptorSetLayout& get_layout(Const::DESCRITPOR_LAYOUT type);

	uint32_t load_texture(std::string texture_path);

	const Texture_View& view_texture(uint32_t texture_id);

	uint32_t load_font(std::string font_path);

	const Font& view_font(uint32_t font_id);

	const Texture_View& view_font_texture(uint32_t font_id);

	uint32_t load_model(std::string path);

	const Model& view_model(uint32_t model_id);

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

	void assert_draw_handle(const Handle& handle);

	void set_draw_dirty(const Handle& handle);

	Handle create_model(
		const std::string& path, const std::string& scene_name = "", const std::string& animation_name = "",
		const glm::vec3& position = {0.f, 0.f, 0.f}, const glm::vec3& scale = {1.f, 1.f, 1.f},
		const glm::vec3& rotation = {0.f, 0.f, 0.f}
	);

	Handle create_model(const Model_3D_Description& model_info);

	Model_3D_Description view_model_description(const Handle& handle);

	Model_3D& modify_model(const Handle& handle);

	Handle create_label(
		const std::string& font_path = "", int font_size = 12, const std::string& text = "", int align = 0,
		glm::vec2 position = {0.f, 0.f}, glm::vec2 scale = {1.f, 1.f}, float rotation = 0.f,
		glm::vec2 anchor_point = {0.f, 0.f}, glm::vec3 color = {1.f, 1.f, 1.f}
	);

	Handle create_label(const Label_2D_Description& label_info);

	Label_2D_Description view_label_description(const Handle& handle);

	Label_2D& modify_label(const Handle& handle);

	Handle create_texture(
		const std::string& texture_path, const Geometry::Texture_Rect_2D& texture_rect = {0.f, 0.f, 1.f, 1.f},
		glm::vec2 position = {0.f, 0.f}, glm::vec2 scale = {1.f, 1.f}, float rotation = 0.f,
		glm::vec2 anchor_point = {0.f, 0.f}, glm::vec3 color = {1.f, 1.f, 1.f}
	);

	Handle create_texture(const Texture_2D_Description& texture_info);

	Texture_2D_Description view_texture_description(const Handle& handle);

	Texture_2D& modify_texture(const Handle& handle);

	Handle create_rectangle(
		float width, float height, glm::vec2 position = {0.f, 0.f}, glm::vec2 scale = {1.f, 1.f}, float rotation = 0.f,
		glm::vec2 anchor_point = {0.f, 0.f}, glm::vec3 color = {1.f, 1.f, 1.f}
	);

	Handle create_rectangle(const Rectangle_2D_Description& rectangle_info);

	Rectangle_2D_Description view_rectangle_description(const Handle& handle);

	Rectangle_2D& modify_rectangle(const Handle& handle);

	Handle create_triangle(
		glm::vec2 first_position, glm::vec2 second_position, glm::vec2 third_position, glm::vec3 color = {1.f, 1.f, 1.f}
	);

	Handle create_triangle(const Triangle_2D_Description& triangle_info);

	Triangle_2D_Description view_triangle_description(const Handle& handle);

	Triangle_2D& modify_triangle(const Handle& handle);

	void remove_draw(const Handle& handle);

} // namespace Vulkan