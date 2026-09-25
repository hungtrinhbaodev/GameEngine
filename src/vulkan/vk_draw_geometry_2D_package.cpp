#include <array>
#include <math_custom.h>
#include <utils.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_draw_geometry_2D_package.h>
#include <vulkan/vk_vertex.h>
#include <vulkan/vk_vertex_input_builder.h>

namespace Vulkan {
	std::vector<uint16_t> Draw_Geometry_2D_Package::TRIANGLE_INDICES = {0, 1, 2};

	void Draw_Geometry_2D_Package::init(
		Ring_Buffer* global_staging_buffer, Static_Buffer* vertices_buffer, Static_Buffer* indices_buffer,
		std::vector<Buffer>& uniform_buffers
	) {
		Draw_Package::init(global_staging_buffer, vertices_buffer, indices_buffer, uniform_buffers);

		/**
		 * Init vertex attributes descriptor.
		 */
		Vertex_Input_Builder vertex_builder = this->make_vertex_2D_builder();

		/**
		 * Create config pipeline.
		 */
		this->pipeline_config.vertex_descriptions = vertex_builder.build_binding_descriptions();
		this->pipeline_config.attribute_descriptions = vertex_builder.build_attribute_descriptions();
		this->pipeline_config.descriptor_set_layouts = this->descriptor_set_layouts;
		this->pipeline_config.vertex_shader_path = Const::PATH_VERT_SHADERD_DRAW_GEOMETRY_2D;
		this->pipeline_config.fragment_shader_path = Const::PATH_FRAG_SHADERD_DRAW_GEOMETRY_2D;
		this->pipeline_config.push_constants_size = sizeof(Push_Constants);

		/**
		 * Make pipeline to this draw package.
		 */
		this->pipeline_info.init(this->pipeline_config);

		/**
		 * Initialize instancing buffer.
		 */
		this->instancing_buffer.init(
			this->global_staging_buffer, Const::INITIALIZE_SIZE_INSTANCING_BUFFER, sizeof(glm::mat4)
		);
	}

	void Draw_Geometry_2D_Package::setup_frist_frame() {
		/**
		 * Request slot indices to triangle.
		 */
		triangle_indices_id =
			this->indices_buffer->upload_data(sizeof(uint16_t) * TRIANGLE_INDICES.size(), TRIANGLE_INDICES.data());
	}

	void Draw_Geometry_2D_Package::draw(VkCommandBuffer command_buffer, VkExtent2D swapchain_extent) {
		Push_Constants constant{glm::vec2(swapchain_extent.width, swapchain_extent.height)};
		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_info.pipeline);
		vkCmdPushConstants(
			command_buffer, pipeline_info.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
			sizeof(Push_Constants), &constant
		);
		/**
		 * Draw all trangles.
		 */
		for (const auto& triangle : triangles) {
			Static_Buffer_Range vertex_range = this->vertices_buffer->view_slot_info(triangle.vertex_id);
			Static_Buffer_Range indices_range = this->indices_buffer->view_slot_info(triangle_indices_id);
			vkCmdDrawIndexed(
				command_buffer, indices_range.size_as<uint16_t>(), 1, indices_range.offset_as<uint16_t>(),
				vertex_range.offset_as<Vertex_2D>(), 0
			);
		}
	}

	void Draw_Geometry_2D_Package::end_frame() {
		for (const auto& triangle : triangles) {
			this->vertices_buffer->remove_data(triangle.vertex_id);
		}
		this->triangles.clear();
	}

	void Draw_Geometry_2D_Package::destroy() {
		Draw_Package::destroy();
		this->instancing_buffer.destroy();
	}

	Const::VERTEX_BUFFER_TYPE Draw_Geometry_2D_Package::get_using_vertex_type() {
		return Const::VERTEX_BUFFER_TYPE::VERTEX_2D;
	}

	void Draw_Geometry_2D_Package::draw_triangle_2D(
		glm::vec2 first_position, glm::vec2 second_position, glm::vec2 third_position, glm::vec3 color
	) {
		if (!Math::is_valid_triangle_with_clockwise(first_position, second_position, third_position)) {
			throw std::runtime_error("Fail to draw triangle: points need to follow clockwise order!");
		}
		Vertex_2D triangle_vertices[] = {{first_position, color}, {second_position, color}, {third_position, color}};
		Triangle_Draw_Information draw_information{
			this->vertices_buffer->upload_data(sizeof(Vertex_2D) * 3, triangle_vertices)
		};
		this->triangles.push_back(draw_information);
	}

} // namespace Vulkan