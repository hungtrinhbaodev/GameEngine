#pragma once

#include <vector>
#include <vulkan/vk_draw_package.h>
#include <vulkan/vk_instance_buffer.h>

namespace Vulkan {

	struct Triangle_Draw_Information {
		uint32_t vertex_id = 0;
	};

	struct Draw_Geometry_2D_Package : public Draw_Package {
		struct Push_Constants {
			glm::vec2 screen_size{0.f, 0.f};
		};

		/**
		 * Triangle draw data
		 */
		static std::vector<uint16_t> TRIANGLE_INDICES;
		std::vector<Triangle_Draw_Information> triangles{};
		uint32_t triangle_indices_id = 0;

		Instance_Buffer instancing_buffer{};

		void init(
			Ring_Buffer* global_staging_buffer, Static_Buffer* vertices_buffer, Static_Buffer* indices_buffer,
			std::vector<Buffer>& uniform_buffers
		) override;
		void setup_frist_frame() override;
		void draw(VkCommandBuffer command_buffer, VkExtent2D swapchain_extent) override;
		void end_frame() override;
		void destroy() override;
		Const::VERTEX_BUFFER_TYPE get_using_vertex_type() override;

		/**
		 * API draw geometry 2D
		 */
		void draw_triangle_2D(
			glm::vec2 first_position, glm::vec2 second_position, glm::vec2 third_position, glm::vec3 color
		);
	};

} // namespace Vulkan