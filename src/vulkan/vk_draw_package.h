#pragma once
#include <vulkan/vk_buffer.h>
#include <vulkan/vk_consts.h>
#include <vulkan/vk_pipeline.h>
#include <vulkan/vk_ring_buffer.h>
#include <vulkan/vk_static_buffer.h>
#include <vulkan/vk_vertex_input_builder.h>
#include <vulkan/vulkan.h>

namespace Vulkan {
	struct Draw_Package {

		Pipeline_Config pipeline_config{};
		Pipeline pipeline_info{};

		std::vector<std::vector<VkDescriptorSet>> descriptors{};
		std::vector<VkDescriptorSetLayout> descriptor_set_layouts{};

		Static_Buffer* vertices_buffer = nullptr;
		Static_Buffer* indices_buffer = nullptr;
		Ring_Buffer* global_staging_buffer = nullptr;

		bool is_setup_first_frame = false;

		virtual void init(
			Ring_Buffer* global_staging_buffer, Static_Buffer* vertices_buffer, Static_Buffer* indices_buffer,
			std::vector<Buffer>& uniform_buffers
		);
		virtual void setup_frist_frame();
		virtual void start_frame();
		virtual void draw(VkCommandBuffer command_buffer, VkExtent2D swapchain_extent);
		virtual void end_frame();
		virtual void destroy();
		virtual Const::VERTEX_BUFFER_TYPE get_using_vertex_type();

		Vertex_Input_Builder make_vertex_2D_builder();
		Vertex_Input_Builder make_vertex_3D_builder();
	};
} // namespace Vulkan