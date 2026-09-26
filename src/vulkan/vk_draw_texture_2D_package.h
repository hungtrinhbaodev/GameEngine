#pragma once
#include <array>
#include <geometry_structs.h>
#include <log.h>
#include <map>
#include <string>
#include <vector>
#include <vulkan/vk_draw_package.h>
#include <vulkan/vk_instance_buffer.h>
#include <vulkan/vk_texture_system.h>
#include <vulkan/vk_vertex.h>

namespace Vulkan {

	struct Texture_2D_Instance_Data {
		glm::vec2 tex_size{0.f, 0.f};
		glm::vec2 translation{0.f, 0.f};
		glm::vec2 scale{1.f, 1.f};
		glm::vec2 anchor{0.f, 0.f};
		std::array<glm::vec2, 4> tex_coord{{{0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}}};
		float rotation = 0.f;
		float z_depth = 0.f;
		friend std::ostream& operator<<(std::ostream& os, const Texture_2D_Instance_Data& instance) {
			std::vector<glm::vec2> tex_coord = {};
			for (int i = 0; i < instance.tex_coord.size(); i++) {
				tex_coord.push_back(instance.tex_coord[i]);
			}
			os << "{Vertex: translation: " << instance.translation << ",  tex_size: " << instance.tex_size
			   << ", anchor: " << instance.anchor << ", tex_coord: " << tex_coord << ", rotation: " << instance.rotation
			   << ", z_depth: " << std::setprecision(std::numeric_limits<float>::max_digits10) << instance.z_depth
			   << "}";
			return os;
		}
	};

	struct Draw_Texture_2D_Package : public Draw_Package {
		struct Push_Constants {
			glm::vec2 screen_size{0.f, 0.f};
			uint32_t texture_index_offset = 0;
		};
		static std::vector<Vertex_2D> TEXTRUE_MAPPING_VERTICES;
		static std::vector<uint16_t> TEXTURE_MAPPING_INDICES;

		float* global_z_depth_2D = nullptr;
		Texture_System* texture_system = nullptr;

		uint32_t texture_vertex_id = 0;
		uint32_t texture_indices_id = 0;
		std::map<uint32_t, std::vector<VkDescriptorSet>> texture_descriptor_sets_at_frame{};
		std::map<uint32_t, std::vector<uint32_t>> texture_instance_by_id;
		Instance_Buffer texture_instance_buffer{};

		void init(
			Ring_Buffer* global_staging_buffer, Static_Buffer* vertices_buffer, Static_Buffer* indices_buffer,
			std::vector<Buffer>& uniform_buffers, float* global_z_depth_2D, Texture_System* texture_system
		);
		void setup_first_frame() override;
		void flush_data() override;
		void draw(VkCommandBuffer command_buffer, VkExtent2D swapchain_extent, uint32_t frame_index) override;
		void end_frame() override;
		void destroy() override;
		Const::VERTEX_BUFFER_TYPE get_using_vertex_type() override;

		/**
		 * API draw texture 2D
		 */
		void draw_texture_2D(
			std::string texture_path, glm::vec2 position, glm::vec2 scale, float rotation = 0,
			glm::vec2 anchor = {0.f, 0.f}, Geometry::Texture_Rect_2D texture_rect = {0.f, 0.f, 1.f, 1.f}
		);
	};
} // namespace Vulkan