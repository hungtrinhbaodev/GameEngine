#include <geometry_structs.h>
#include <log.h>
#include <math_custom.h>
#include <utils.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_descriptor.h>
#include <vulkan/vk_draw_model_3D_package.h>
#include <vulkan/vk_structs.h>

namespace Vulkan {

	void Draw_Model_3D_Package::init(
		Ring_Buffer* global_staging_buffer, Static_Buffer* vertices_buffer, Static_Buffer* indices_buffer,
		std::vector<Buffer>& uniform_buffers, Model_3D_System* model_system, Texture_System* texture_system
	) {
		Draw_Package::init(global_staging_buffer, vertices_buffer, indices_buffer, uniform_buffers);
		this->model_system = model_system;
		this->texture_system = texture_system;

		Vertex_Input_Builder vertex_builder = this->make_vertex_3D_builder();
		vertex_builder.add_binding_description(1, sizeof(glm::mat4), VK_VERTEX_INPUT_RATE_INSTANCE)
			.add_mat4_attribute_description(1, 0);

		Descriptor_Set_Layout_Builder texture_layout_builder{};
		texture_layout_builder.add_binding(
			0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT
		);
		this->descriptor_set_layouts.push_back(texture_layout_builder.build());

		this->pipeline_config.attribute_descriptions = vertex_builder.build_attribute_descriptions();
		this->pipeline_config.vertex_binding_descriptions = vertex_builder.build_binding_descriptions();
		this->pipeline_config.descriptor_set_layouts = this->descriptor_set_layouts;
		this->pipeline_config.vertex_shader_path = Const::PATH_VERT_SHADERD_DRAW_MODEL_3D;
		this->pipeline_config.fragment_shader_path = Const::PATH_FRAG_SHADERD_DRAW_MODEL_3D;
		this->pipeline_config.depth_compare_op = VK_COMPARE_OP_LESS;
		this->pipeline_config.triangle_trip_order = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		this->pipeline_config.push_constants_size = sizeof(Push_Constants);

		this->pipeline_info.init(this->pipeline_config);

		this->model_instance_buffer.init(
			this->global_staging_buffer, Const::INITIALIZE_SIZE_INSTANCING_BUFFER, sizeof(glm::mat4),
			Vulkan::physical_device, Vulkan::device
		);
	}

	void Draw_Model_3D_Package::flush_data() {
		this->model_instance_buffer.flush_data();
	}

	void Draw_Model_3D_Package::draw(
		VkCommandBuffer command_buffer, VkExtent2D swapchain_extent, uint32_t frame_index
	) {
		VkDeviceSize instance_buffer_offset = 0;
		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_info.pipeline);
		vkCmdBindVertexBuffers(
			command_buffer, 1, 1, &this->model_instance_buffer.inner_buffer.buffer, &instance_buffer_offset
		);
		std::vector<VkDescriptorSet>& uiniform_descriptor_sets = this->descriptors[0];
		vkCmdBindDescriptorSets(
			command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_info.layout, 0, 1,
			&uiniform_descriptor_sets[frame_index], 0, VK_NULL_HANDLE
		);
		for (const auto& [model_id, instances] : model_instances) {
			const Model_Information& model_info = this->model_system->view_model(model_id);
			const uint32_t draw_scene_index = model_info.draw_scene_index;
			const std::vector<Model_Mesh_Information>& meshes = model_info.meshes;
			for (uint32_t i = 0; i < meshes.size(); i++) {
				const Model_Mesh_Information& mesh = meshes[i];
				const std::vector<glm::mat4>& mesh_transforms =
					model_info.global_meshes_transform.at(draw_scene_index).at(i);
				glm::vec3 mesh_origin = mesh.get_mesh_origin();
				for (const glm::mat4& transform : mesh_transforms) {
					glm::mat4 final_transform = transform * Math::make_translation(-mesh_origin);
					vkCmdPushConstants(
						command_buffer, pipeline_info.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
						0, sizeof(glm::mat4), &final_transform
					);
					for (const Primitive_Buffer_Range& primitive : mesh.primitives) {
						Static_Buffer_Range vertex_range = this->vertices_buffer->view_slot_info(primitive.vertex_id);
						Static_Buffer_Range indices_range = this->indices_buffer->view_slot_info(primitive.indices_id);
						VkDescriptorSet texture_descriptor =
							this->texture_descriptor_sets[primitive.texture_id][frame_index];
						vkCmdBindDescriptorSets(
							command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline_info.layout, 1, 1,
							&texture_descriptor, 0, VK_NULL_HANDLE
						);
						for (const uint32_t& instance_id : instances) {
							uint32_t instance_index = this->model_instance_buffer.get_index_by(instance_id);
							vkCmdDrawIndexed(
								command_buffer, indices_range.size_as<uint32_t>(), 1,
								indices_range.offset_as<uint32_t>(), vertex_range.offset_as<Geometry::Vertex_3D>(),
								instance_index
							);
						}
					}
				}
			}
		}
	}

	void Draw_Model_3D_Package::end_frame() {
		for (auto& [model_id, instances] : this->model_instances) {
			for (auto& instance_id : instances) {
				model_instance_buffer.remove_data(instance_id);
			}
		}
		this->model_instances.clear();
	}

	void Draw_Model_3D_Package::destroy() {
		Draw_Package::destroy();
		this->model_instance_buffer.destroy();
	}

	Const::VERTEX_BUFFER_TYPE Draw_Model_3D_Package::get_using_vertex_type() {
		return Const::VERTEX_BUFFER_TYPE::VERTEX_3D;
	}

	void Draw_Model_3D_Package::draw_model_3D(
		std::string path, glm::vec3 position, glm::vec3 scale, glm::vec3 rotation
	) {
		uint32_t model_id = this->model_system->load_model(path);
		if (this->model_instances.find(model_id) == this->model_instances.end()) {
			this->model_instances[model_id] = {};
		}
		const Model_Information& model = this->model_system->view_model(model_id);
		for (const Model_Mesh_Information& mesh : model.meshes) {
			for (const Primitive_Buffer_Range& primitive : mesh.primitives) {
				Texture_View view = this->texture_system->view_texture(primitive.texture_id);
				if (this->texture_descriptor_sets.find(primitive.texture_id) == this->texture_descriptor_sets.end()) {
					std::vector<VkDescriptorSet> texture_descriptor_sets{};
					for (int i = 0; i < Const::MAX_FRAMES_IN_FLIGHT; i++) {
						std::vector<VkDescriptorSet> descriptor_sets = Structs::make_descriptor_set(
							this->descriptor_pools[i], 1, &this->descriptor_set_layouts[1], this->device
						);
						Descriptor_Set_Writer writer{};
						writer
							.add_image_write(0, 1, &view.image.get_descriptor_info(view.slot_index), descriptor_sets[0])
							.write();
						texture_descriptor_sets.push_back(descriptor_sets[0]);
					}
					this->texture_descriptor_sets[primitive.texture_id] = std::move(texture_descriptor_sets);
				}
			}
		}
		glm::mat4 instance{1.f};
		instance *= (Math::make_translation(position) * Math::make_rotation(rotation) * Math::make_scale(scale));
		this->model_instances[model_id].push_back(this->model_instance_buffer.add_data(&instance));
	}
} // namespace Vulkan