#include <math_custom.h>
#include <utils.h>
#include <vulkan/vk_descriptor.h>
#include <vulkan/vk_draw_texture_2D_package.h>
#include <vulkan/vk_structs.h>
#include <vulkan/vk_texture_system.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

	std::vector<Vertex_2D> Draw_Texture_2D_Package::TEXTRUE_MAPPING_VERTICES = {
		{{{0.f, 0.f}}, {{0.f, 1.f}}, {{1.f, 1.f}}, {{1.f, 0.f}}}
	};

	std::vector<uint16_t> Draw_Texture_2D_Package::TEXTURE_MAPPING_INDICES = {0, 1, 2, 2, 3, 0};

	void Draw_Texture_2D_Package::init(
		Ring_Buffer* global_staging_buffer, Static_Buffer* vertices_buffer, Static_Buffer* indices_buffer,
		std::vector<Buffer>& uniform_buffers, float* global_z_depth_2D, Texture_System* texture_system
	) {
		Draw_Package::init(global_staging_buffer, vertices_buffer, indices_buffer, uniform_buffers);
		this->global_z_depth_2D = global_z_depth_2D;
		this->texture_system = texture_system;

		Vertex_Input_Builder vertex_builder = this->make_vertex_2D_builder();
		vertex_builder.add_binding_description(1, sizeof(Texture_2D_Instance_Data), VK_VERTEX_INPUT_RATE_INSTANCE)
			.add_attribute_description(1, VK_FORMAT_R32G32_SFLOAT, offsetof(Texture_2D_Instance_Data, tex_size))
			.add_attribute_description(1, VK_FORMAT_R32G32_SFLOAT, offsetof(Texture_2D_Instance_Data, translation))
			.add_attribute_description(1, VK_FORMAT_R32G32_SFLOAT, offsetof(Texture_2D_Instance_Data, scale))
			.add_attribute_description(1, VK_FORMAT_R32G32_SFLOAT, offsetof(Texture_2D_Instance_Data, anchor))
			.add_array_vec2_attribute_description(1, offsetof(Texture_2D_Instance_Data, tex_coord), 4)
			.add_attribute_description(1, VK_FORMAT_R32_SFLOAT, offsetof(Texture_2D_Instance_Data, rotation))
			.add_attribute_description(1, VK_FORMAT_R32_SFLOAT, offsetof(Texture_2D_Instance_Data, z_depth));

		Descriptor_Set_Layout_Builder layout_builder{};
		layout_builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
		this->descriptor_set_layouts.push_back(layout_builder.build());

		this->pipeline_config.attribute_descriptions = vertex_builder.build_attribute_descriptions();
		this->pipeline_config.vertex_descriptions = vertex_builder.build_binding_descriptions();
		this->pipeline_config.descriptor_set_layouts = this->descriptor_set_layouts;
		this->pipeline_config.depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
		this->pipeline_config.push_constants_size = sizeof(Push_Constants);
		this->pipeline_config.vertex_shader_path = Const::PATH_VERT_SHADERD_DRAW_TEXTURE_2D;
		this->pipeline_config.fragment_shader_path = Const::PATH_FRAG_SHADERD_DRAW_TEXTURE_2D;

		this->pipeline_info.init(this->pipeline_config);
		/**
		 * Initialize instance texture 2D
		 */
		this->texture_instance_buffer.init(
			this->global_staging_buffer, Const::INITIALIZE_SIZE_INSTANCING_BUFFER, sizeof(Texture_2D_Instance_Data)
		);
	}

	void Draw_Texture_2D_Package::setup_first_frame() {
		this->texture_vertex_id = this->vertices_buffer->upload_data(
			sizeof(Vertex_2D) * TEXTRUE_MAPPING_VERTICES.size(), TEXTRUE_MAPPING_VERTICES.data()
		);
		this->texture_indices_id = this->indices_buffer->upload_data(
			sizeof(uint16_t) * TEXTURE_MAPPING_INDICES.size(), TEXTURE_MAPPING_INDICES.data()
		);
	}

	void Draw_Texture_2D_Package::flush_data() {
		this->texture_instance_buffer.flush_data();
	}

	void Draw_Texture_2D_Package::draw(
		VkCommandBuffer command_buffer, VkExtent2D swapchain_extent, uint32_t frame_index
	) {
		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline_info.pipeline);
		VkDeviceSize instance_buffer_offset = 0;
		vkCmdBindVertexBuffers(
			command_buffer, 1, 1, &this->texture_instance_buffer.inner_buffer.buffer, &instance_buffer_offset
		);
		glm::vec2 screen_size{swapchain_extent.width, swapchain_extent.height};
		vkCmdPushConstants(
			command_buffer, pipeline_info.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
			sizeof(glm::vec2), &screen_size
		);
		/**
		 * Draw all textures.
		 */
		Static_Buffer_Range vertex_range = this->vertices_buffer->view_slot_info(this->texture_vertex_id);
		Static_Buffer_Range indices_range = this->indices_buffer->view_slot_info(this->texture_indices_id);
		uint32_t texture_index_offset = vertex_range.offset_as<Vertex_2D>();
		vkCmdPushConstants(
			command_buffer, pipeline_info.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			offsetof(Push_Constants, texture_index_offset), sizeof(uint32_t), &texture_index_offset
		);
		for (const auto& [texture_id, instances] : this->texture_instance_by_id) {
			std::vector<VkDescriptorSet>& descriptor_sets = this->texture_descriptor_sets_at_frame[texture_id];
			VkDescriptorSet using_descriptor = descriptor_sets[frame_index];
			vkCmdBindDescriptorSets(
				command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_info.layout, 1, 1, &using_descriptor, 0,
				VK_NULL_HANDLE
			);
			for (const auto instance_id : instances) {
				uint32_t instance_index = this->texture_instance_buffer.get_index_by(instance_id);
				vkCmdDrawIndexed(
					command_buffer, indices_range.size_as<uint16_t>(), 1, indices_range.offset_as<uint16_t>(),
					vertex_range.offset_as<Vertex_2D>(), instance_index
				);
			}
		}
	}

	void Draw_Texture_2D_Package::end_frame() {
		for (const auto& [texture_id, instance_ids] : this->texture_instance_by_id) {
			for (const auto instance_id : instance_ids) {
				this->texture_instance_buffer.remove_data(instance_id);
			}
		}
		this->texture_instance_by_id.clear();
	}

	void Draw_Texture_2D_Package::destroy() {
		this->texture_instance_buffer.destroy();
		Draw_Package::destroy();
	}

	Const::VERTEX_BUFFER_TYPE Draw_Texture_2D_Package::get_using_vertex_type() {
		return Const::VERTEX_BUFFER_TYPE::VERTEX_2D;
	}

	void Draw_Texture_2D_Package::draw_texture_2D(
		std::string texture_path, glm::vec2 position, glm::vec2 scale, float rotation, glm::vec2 anchor,
		Geometry::Texture_Rect_2D texture_rect
	) {
		uint32_t texture_id = this->texture_system->load_texture(::Utils::get_root_path() + texture_path);
		Texture_View texture_view = this->texture_system->view_texture(texture_id);
		if (this->texture_descriptor_sets_at_frame.find(texture_id) == this->texture_descriptor_sets_at_frame.end()) {
			/**
			 * Allocate new descriptor set to this new texture.
			 */
			std::vector<VkDescriptorSet> texture_descriptor_sets{};
			for (int i = 0; i < Const::MAX_FRAMES_IN_FLIGHT; i++) {
				std::vector<VkDescriptorSet> texture_descriptor_set = Structs::make_descriptor_set(
					this->descriptor_pools[i], 1, &this->descriptor_set_layouts[1], this->device
				);
				Descriptor_Set_Writer writer{};
				writer.add_image_write(0, &texture_view.image.descriptor, texture_descriptor_set[0]).write();
				texture_descriptor_sets.push_back(texture_descriptor_set[0]);
			}
			this->texture_descriptor_sets_at_frame[texture_id] = texture_descriptor_sets;
		}
		glm::vec2 tex_size = {
			texture_rect.ratio_width * texture_view.image.width, texture_rect.ratio_height * texture_view.image.height
		};
		float z_depth = Utils::calculate_z_depth_2D(*this->global_z_depth_2D);
		Texture_2D_Instance_Data instance_data{
			tex_size,
			position,
			scale,
			anchor,
			Math::make_tex_coord_from(texture_rect),
			rotation,
			Utils::calculate_z_depth_2D(*this->global_z_depth_2D),
		};
		if (this->texture_instance_by_id.find(texture_id) == this->texture_instance_by_id.end()) {
			this->texture_instance_by_id[texture_id] = {};
		}
		this->texture_instance_by_id[texture_id].push_back(this->texture_instance_buffer.add_data(&instance_data));
		*this->global_z_depth_2D += 1.f;
	}
} // namespace Vulkan