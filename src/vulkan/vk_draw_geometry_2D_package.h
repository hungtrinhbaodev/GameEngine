#pragma once

#include <iostream>
#include <vector>
#include <vulkan/vk_draw_package.h>
#include <vulkan/vk_instance_buffer.h>
#include <vulkan/vk_vertex.h>

namespace Vulkan {

	struct Geometry_2D_Instance_Data {
		glm::vec2 translation{0.f, 0.f};
		glm::vec2 size{1.f, 1.f};
		glm::vec2 anchor{0.f, 0.f};
		glm::vec3 color{1.f, 1.f, 1.f};
		float rotation = 0.f;
		float z_depth = 0.f;
		friend std::ostream& operator<<(std::ostream& os, const Geometry_2D_Instance_Data& instance) {
			os << "{Vertex: translation: " << instance.translation << ",  scale: " << instance.size
			   << ", anchor: " << instance.anchor << ", color: " << instance.color
			   << ", rotation: " << instance.rotation
			   << ", z_depth: " << std::setprecision(std::numeric_limits<float>::max_digits10) << instance.z_depth
			   << "}";
			return os;
		}
	};

	struct Triangle_Draw_Information {
		uint32_t vertex_id = 0;
		float z_depth = 0.f;
	};

	struct Rectangle_Draw_Information {
		uint32_t instance_id = 0;
	};

	struct Draw_Geometry_2D_Package : public Draw_Package {
		struct Push_Constants {
			glm::vec2 screen_size{0.f, 0.f};
			uint32_t draw_type = -1;
			float triangle_z_depth = 0.f;
		};
		static const uint32_t TRIANGLE_DRAW_TYPE;
		static const uint32_t RECTANGLE_DRAW_TYPE;

		float* global_z_depth_2D = nullptr;

		/**
		 * Triangle draw data
		 */
		static std::vector<uint16_t> TRIANGLE_INDICES;
		uint32_t triangle_indices_id = 0;
		Instance_Buffer triangle_instance_buffer{};
		std::vector<Triangle_Draw_Information> triangles{};

		/**
		 * Rectangle draw data
		 */
		static std::vector<uint16_t> RECTANGLE_INDICES;
		static std::vector<Vertex_2D> RECTANGLE_VERTICES;
		uint32_t rectangle_vertex_id = 0;
		uint32_t rectangle_indices_id = 0;
		std::vector<Rectangle_Draw_Information> rectangles{};
		Instance_Buffer rectangle_instance_buffer{};

		void init(
			Ring_Buffer* global_staging_buffer, Static_Buffer* vertices_buffer, Static_Buffer* indices_buffer,
			std::vector<Buffer>& uniform_buffers, float* global_z_depth_2D
		);
		void setup_first_frame() override;
		void flush_data() override;
		void draw(VkCommandBuffer command_buffer, VkExtent2D swapchain_extent, uint32_t frame_index) override;
		void end_frame() override;
		void destroy() override;
		Const::VERTEX_BUFFER_TYPE get_using_vertex_type() override;

		/**
		 * API draw geometry 2D
		 */
		void draw_triangle_2D(
			glm::vec2 first_position, glm::vec2 second_position, glm::vec2 third_position, glm::vec3 color
		);
		void draw_rectangle_2D(
			float x, float y, float width, float height, glm::vec3 color, float rotation = 0,
			glm::vec2 anchor_point = {0.f, 0.f}
		);
	};

} // namespace Vulkan