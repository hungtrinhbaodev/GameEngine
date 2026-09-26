#include <algorithm>
#include <array>
#include <math_custom.h>
#include <utils.h>
#include <vulkan/vk_draw_geometry_2D_package.h>
#include <vulkan/vk_utils.h>
#include <vulkan/vk_vertex_input_builder.h>

namespace Vulkan {

	const uint32_t Draw_Geometry_2D_Package::TRIANGLE_DRAW_TYPE = 0;

	const uint32_t Draw_Geometry_2D_Package::RECTANGLE_DRAW_TYPE = 1;

	std::vector<uint16_t> Draw_Geometry_2D_Package::TRIANGLE_INDICES = {0, 1, 2};

	std::vector<uint16_t> Draw_Geometry_2D_Package::RECTANGLE_INDICES = {0, 1, 2, 2, 3, 0};

	std::vector<Vertex_2D> Draw_Geometry_2D_Package::RECTANGLE_VERTICES = {
		{{0.f, 0.f}}, {{0.f, 1.f}}, {{1.f, 1.f}}, {{1.f, 0.f}}
	};

	void Draw_Geometry_2D_Package::init(
		Ring_Buffer* global_staging_buffer, Static_Buffer* vertices_buffer, Static_Buffer* indices_buffer,
		std::vector<Buffer>& uniform_buffers, float* global_z_depth_2D
	) {
		Draw_Package::init(global_staging_buffer, vertices_buffer, indices_buffer, uniform_buffers);
		this->global_z_depth_2D = global_z_depth_2D;

		/**
		 * Init vertex attributes descriptor.
		 */
		Vertex_Input_Builder vertex_builder = this->make_vertex_2D_builder();
		vertex_builder.add_binding_description(1, sizeof(Geometry_2D_Instance_Data), VK_VERTEX_INPUT_RATE_INSTANCE)
			.add_attribute_description(1, VK_FORMAT_R32G32_SFLOAT, offsetof(Geometry_2D_Instance_Data, translation))
			.add_attribute_description(1, VK_FORMAT_R32G32_SFLOAT, offsetof(Geometry_2D_Instance_Data, size))
			.add_attribute_description(1, VK_FORMAT_R32G32_SFLOAT, offsetof(Geometry_2D_Instance_Data, anchor))
			.add_attribute_description(1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Geometry_2D_Instance_Data, color))
			.add_attribute_description(1, VK_FORMAT_R32_SFLOAT, offsetof(Geometry_2D_Instance_Data, z_depth))
			.add_attribute_description(1, VK_FORMAT_R32_SFLOAT, offsetof(Geometry_2D_Instance_Data, rotation));

		/**
		 * Create config pipeline.
		 */
		this->pipeline_config.vertex_descriptions = vertex_builder.build_binding_descriptions();
		this->pipeline_config.attribute_descriptions = vertex_builder.build_attribute_descriptions();
		this->pipeline_config.descriptor_set_layouts = this->descriptor_set_layouts;
		this->pipeline_config.vertex_shader_path = Const::PATH_VERT_SHADERD_DRAW_GEOMETRY_2D;
		this->pipeline_config.fragment_shader_path = Const::PATH_FRAG_SHADERD_DRAW_GEOMETRY_2D;
		this->pipeline_config.push_constants_size = sizeof(Push_Constants);
		this->pipeline_config.depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;

		/**
		 * Make pipeline to this draw package.
		 */
		this->pipeline_info.init(this->pipeline_config);

		/**
		 * Init triangle instance buffer.
		 */
		this->triangle_instance_buffer.init(
			this->global_staging_buffer, sizeof(Geometry_2D_Instance_Data), sizeof(Geometry_2D_Instance_Data)
		);

		/**
		 * Init rectangle instance buffer.
		 */
		this->rectangle_instance_buffer.init(
			this->global_staging_buffer, Const::INITIALIZE_SIZE_INSTANCING_BUFFER, sizeof(Geometry_2D_Instance_Data)
		);
	}

	void Draw_Geometry_2D_Package::flush_data() {
		/**
		 * Flush all instancing data to GPU.
		 */
		this->triangle_instance_buffer.flush_data();
		this->rectangle_instance_buffer.flush_data();
	}

	void Draw_Geometry_2D_Package::setup_first_frame() {
		/**
		 * Request slot indices to triangle.
		 */
		this->triangle_indices_id =
			this->indices_buffer->upload_data(sizeof(uint16_t) * TRIANGLE_INDICES.size(), TRIANGLE_INDICES.data());
		Geometry_2D_Instance_Data triangle_default_instance{};
		this->triangle_instance_buffer.add_data(&triangle_default_instance);

		this->rectangle_vertex_id = this->vertices_buffer->upload_data(
			sizeof(Vertex_2D) * RECTANGLE_VERTICES.size(), RECTANGLE_VERTICES.data()
		);
		this->rectangle_indices_id =
			this->indices_buffer->upload_data(sizeof(uint16_t) * RECTANGLE_INDICES.size(), RECTANGLE_INDICES.data());
	}

	void Draw_Geometry_2D_Package::draw(
		VkCommandBuffer command_buffer, VkExtent2D swapchain_extent, uint32_t frame_index
	) {
		/**
		 * Bind pipeline use to draw.
		 */
		glm::vec2 screen_size = glm::vec2(swapchain_extent.width, swapchain_extent.height);
		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_info.pipeline);
		vkCmdPushConstants(
			command_buffer, pipeline_info.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
			sizeof(glm::vec2), &screen_size
		);
		/**
		 * Draw all trangles.
		 */
		VkDeviceSize instance_buffer_offset = 0;
		vkCmdBindVertexBuffers(
			command_buffer, 1, 1, &triangle_instance_buffer.inner_buffer.buffer, &instance_buffer_offset
		);
		vkCmdPushConstants(
			command_buffer, pipeline_info.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			offsetof(Push_Constants, draw_type), sizeof(uint32_t), &TRIANGLE_DRAW_TYPE
		);
		Static_Buffer_Range indices_range = this->indices_buffer->view_slot_info(triangle_indices_id);
		for (const auto& triangle : triangles) {
			Static_Buffer_Range vertex_range = this->vertices_buffer->view_slot_info(triangle.vertex_id);
			/**
			 * @Note: Because we don't use instancing buffer to draw
			 * triangle so the z depth will be send throw push
			 * constants
			 */
			vkCmdPushConstants(
				command_buffer, pipeline_info.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				offsetof(Push_Constants, triangle_z_depth), sizeof(float), &triangle.z_depth
			);
			vkCmdDrawIndexed(
				command_buffer, indices_range.size_as<uint16_t>(), 1, indices_range.offset_as<uint16_t>(),
				vertex_range.offset_as<Vertex_2D>(), 0
			);
		}
		/**
		 * Draw all rectangles.
		 */
		vkCmdBindVertexBuffers(
			command_buffer, 1, 1, &rectangle_instance_buffer.inner_buffer.buffer, &instance_buffer_offset
		);
		vkCmdPushConstants(
			command_buffer, pipeline_info.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			offsetof(Push_Constants, draw_type), sizeof(uint32_t), &RECTANGLE_DRAW_TYPE
		);
		Static_Buffer_Range vertex_range = this->vertices_buffer->view_slot_info(rectangle_vertex_id);
		indices_range = this->indices_buffer->view_slot_info(rectangle_indices_id);
		vkCmdDrawIndexed(
			command_buffer, indices_range.size_as<uint16_t>(), this->rectangle_instance_buffer.number_instance,
			indices_range.offset_as<uint16_t>(), vertex_range.offset_as<Vertex_2D>(), 0
		);
	}

	void Draw_Geometry_2D_Package::end_frame() {
		for (const auto& triangle : this->triangles) {
			this->vertices_buffer->remove_data(triangle.vertex_id);
		}
		this->triangles.clear();

		for (const auto& rectangle : this->rectangles) {
			this->rectangle_instance_buffer.remove_data(rectangle.instance_id);
		}
		this->rectangles.clear();
	}

	void Draw_Geometry_2D_Package::destroy() {
		Draw_Package::destroy();
		this->triangle_instance_buffer.destroy();
		this->rectangle_instance_buffer.destroy();
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
			this->vertices_buffer->upload_data(sizeof(Vertex_2D) * 3, triangle_vertices),
			Utils::calculate_z_depth_2D(*this->global_z_depth_2D)
		};
		this->triangles.push_back(draw_information);
		*this->global_z_depth_2D += 1.f;
	}

	void Draw_Geometry_2D_Package::draw_rectangle_2D(
		float x, float y, float width, float height, glm::vec3 color, float rotation, glm::vec2 anchor_point
	) {
		glm::vec2 size = {width, height};
		glm::vec2 translation = {x, y};
		Geometry_2D_Instance_Data instance{
			translation, size, anchor_point, color, rotation, Utils::calculate_z_depth_2D(*this->global_z_depth_2D),
		};
		this->rectangles.push_back({this->rectangle_instance_buffer.add_data(&instance)});
		*this->global_z_depth_2D += 1.f;
	}

} // namespace Vulkan