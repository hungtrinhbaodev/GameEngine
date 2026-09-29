#pragma once
#include <glm/glm.hpp>
#include <map>
#include <vulkan/vk_draw_package.h>
#include <vulkan/vk_instance_buffer.h>
#include <vulkan/vk_model_3D_system.h>

namespace Vulkan {

	struct Draw_Model_3D_Package : public Draw_Package {
		struct Push_Constants {
			glm::mat4 mesh_transform{1.f};
		};

		Texture_System* texture_system = nullptr;
		Model_3D_System* model_system = nullptr;
		Instance_Buffer model_instance_buffer{};
		std::map<uint32_t, std::vector<uint32_t>> model_instances;
		std::map<uint32_t, std::vector<VkDescriptorSet>> texture_descriptor_sets;

		void init(
			Ring_Buffer* global_staging_buffer, Static_Buffer* vertices_buffer, Static_Buffer* indices_buffer,
			std::vector<Buffer>& uniform_buffers, Model_3D_System* model_system, Texture_System* texture_system
		);
		void flush_data() override;
		void draw(VkCommandBuffer command_buffer, VkExtent2D swapchain_extent, uint32_t frame_index) override;
		void end_frame() override;
		void destroy() override;
		Const::VERTEX_BUFFER_TYPE get_using_vertex_type() override;
		/**
		 * API draw model 3D
		 */
		void draw_model_3D(std::string path, glm::vec3 position, glm::vec3 scale, glm::vec3 rotation);
	};
} // namespace Vulkan