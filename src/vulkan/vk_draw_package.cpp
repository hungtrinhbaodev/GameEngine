#include <vulkan/vk_consts.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_descriptor.h>
#include <vulkan/vk_draw_package.h>
#include <vulkan/vk_structs.h>
#include <vulkan/vk_vertex.h>

namespace Vulkan {

	Draw_Package::Draw_Package() {
		this->device = Vulkan::device;
		this->descriptor_pools = Vulkan::descriptor_pools;
	}

	Draw_Package::~Draw_Package() {}

	void Draw_Package::init(
		Ring_Buffer* global_staging_buffer, Static_Buffer* vertices_buffer, Static_Buffer* indices_buffer,
		std::vector<Buffer>& uniform_buffers
	) {
		this->global_staging_buffer = global_staging_buffer;
		this->vertices_buffer = vertices_buffer;
		this->indices_buffer = indices_buffer;

		/**
		 * Init descriptor set layout start with one
		 * layout that our uniform_buffer binding
		 */
		Descriptor_Set_Layout_Builder layout_builder{};
		layout_builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
		this->descriptor_set_layouts.push_back(layout_builder.build());

		/**
		 * Init descriptor sets to uniform buffer
		 */
		for (int i = 0; i < Const::MAX_FRAMES_IN_FLIGHT; i++) {
			this->descriptors.push_back({});
			Buffer& uniform_buffer = uniform_buffers[i];
			std::vector<VkDescriptorSet> uniform_descriptor_set =
				Structs::make_descriptor_set(descriptor_pools[i], 1, &this->descriptor_set_layouts[0], device);
			Descriptor_Set_Writer writer{};
			writer.add_buffer_write(0, &uniform_buffer.descriptor, uniform_descriptor_set[0]);
			this->descriptors[i].push_back(uniform_descriptor_set[0]);
		}

		this->pipeline_config.depth_image = Vulkan::depth_image;
		this->pipeline_config.swapchain_extent = Vulkan::swapchain_extent;
		this->pipeline_config.render_pass = Vulkan::render_pass;
		this->pipeline_config.device = Vulkan::device;
	}

	void Draw_Package::setup_first_frame() {}

	void Draw_Package::start_frame() {}

	void Draw_Package::flush_data() {}

	void Draw_Package::draw(VkCommandBuffer command_buffer, VkExtent2D swapchain_extent, uint32_t frame_index) {}

	void Draw_Package::end_frame() {}

	void Draw_Package::destroy() {
		pipeline_info.destroy();
		for (const auto& descriptor_set_layout : this->descriptor_set_layouts) {
			vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);
		}
	}

	Vertex_Input_Builder Draw_Package::make_vertex_2D_builder() {
		Vertex_Input_Builder vertex_builder{};
		vertex_builder.add_binding_description(0, sizeof(Vertex_2D), VK_VERTEX_INPUT_RATE_VERTEX)
			.add_attribute_description(0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex_2D, position))
			.add_attribute_description(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex_2D, color));
		return vertex_builder;
	}

	Vertex_Input_Builder Draw_Package::make_vertex_3D_builder() {
		return {};
	}

	Const::VERTEX_BUFFER_TYPE Draw_Package::get_using_vertex_type() {
		return Const::VERTEX_BUFFER_TYPE::NONE;
	}
} // namespace Vulkan