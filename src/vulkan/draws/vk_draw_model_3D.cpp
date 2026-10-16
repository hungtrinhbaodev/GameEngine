#include <geometry_structs.h>
#include <id_generator.h>
#include <math_custom.h>
#include <sparse_set.h>
#include <vulkan/draws/vk_draw_model_3D.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_descriptor.h>
#include <vulkan/vk_draw.h>
#include <vulkan/vk_pipeline.h>
#include <vulkan/vk_uniform.h>
#include <vulkan/vk_utils.h>
#include <vulkan/vk_vertex_input_builder.h>

namespace Vulkan {

	namespace Model_3D {

		struct Instance_Data {
			uint32_t storage_offset = 0;
			uint32_t mesh_transform_storage_offset = 0;
			uint32_t texture_storage_offset = 0;
			friend std::ostream& operator<<(std::ostream& os, const Instance_Data& instance) {
				os << "{Model_3D::Instance_Data: storage_offset: " << instance.storage_offset
				   << ", mesh_transform_storage_offset: " << instance.mesh_transform_storage_offset
				   << ", texture_storage_offset: " << instance.texture_storage_offset << "}";
				return os;
			}
		};

		struct Storage_Data {
			glm::mat4 transform{1.f};
			friend std::ostream& operator<<(std::ostream& os, const Storage_Data& instance) {
				os << "{Model_3D::Storage_Data: transform: " << instance.transform << "}";
				return os;
			}
		};

		struct Storage_Texture_Data {
			int bucket_index = 0;
			int slot_index = 0;
			glm::vec2 tex_scale = {1.f, 1.f};
		};

		struct Push_Constants {
			glm::mat4 mesh_transform{1.f};
			int bucket_index = 0;
			int slot_index = 0;
		};

		struct Draw_Data {
			uint32_t storage_id = 0;
			std::vector<uint32_t> instance_ids{};
			std::vector<uint32_t> storage_texture_ids{};
		};

		Id_Generator unique_id_generator{};

		Pipeline_Config pipeline_config{};

		Pipeline pipeline{};

		std::vector<VkDescriptorSetLayout> layouts{};

		Sparse_Set<std::vector<VkDescriptorSet>> texture_descriptor_sets{};

		std::vector<VkDescriptorSet> texture_bucket_descriptor_sets{};

		std::vector<VkDescriptorSet> default_texture_descriptor_sets{};

		/**
		 * @Note: now don't have camera so take the uniform buffer here.
		 */
		std::vector<Buffer> uniform_buffers = {};

		std::vector<uint32_t> uniform_buffer_ids{};

		std::vector<VkDescriptorSet> uniform_descriptor_sets{};

		Sparse_Set<Draw_Data> draws{};

		bool is_material_equal(uint32_t a, uint32_t b) {
			return a == b;
		}

		size_t get_instance_size() {
			return sizeof(Instance_Data);
		}

		void init() {
			SSBO_Buffer& ssbo = get_ssbo();
			size_t uniform_size = sizeof(Uniform);
			for (int i = 0; i < Const::MAX_FRAMES_IN_FLIGHT; i++) {
				Buffer uniform_buffer{};
				uniform_buffer.make_buffer(
					uniform_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physical_device, device
				);
				uniform_buffers.push_back(uniform_buffer);
				Uniform uniform{};
				uniform_buffer_ids.push_back(ssbo.upload_data(&uniform, sizeof(Uniform)));
			}

			Vertex_Input_Builder vertex_builder{};
			vertex_builder.add_binding_description(0, sizeof(Geometry::Vertex_3D), VK_VERTEX_INPUT_RATE_VERTEX)
				.add_attribute_description(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Geometry::Vertex_3D, position))
				.add_attribute_description(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Geometry::Vertex_3D, normal))
				.add_attribute_description(0, VK_FORMAT_R32G32_SFLOAT, offsetof(Geometry::Vertex_3D, tex_coord))
				.add_attribute_description(0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Geometry::Vertex_3D, tangent))
				.add_attribute_description(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Geometry::Vertex_3D, color));
			vertex_builder.add_binding_description(1, sizeof(Instance_Data), VK_VERTEX_INPUT_RATE_INSTANCE)
				.add_attribute_description(1, VK_FORMAT_R32_UINT, offsetof(Instance_Data, storage_offset))
				.add_attribute_description(
					1, VK_FORMAT_R32_UINT, offsetof(Instance_Data, mesh_transform_storage_offset)
				)
				.add_attribute_description(1, VK_FORMAT_R32_UINT, offsetof(Instance_Data, texture_storage_offset));

			layouts.push_back(get_layout(Const::DESCRITPOR_LAYOUT::STORAGE_BUFFER));
			layouts.push_back(get_layout(Const::DESCRITPOR_LAYOUT::IMAGE_SAMPLER));
			if (Const::ENABLED_TEXTURE_BUCKETS) {
				layouts.push_back(get_layout(Const::DESCRITPOR_LAYOUT::BUCKET_IMAGE_SAMPLER));
			} else {
				layouts.push_back(get_layout(Const::DESCRITPOR_LAYOUT::EMPTY));
			}
			layouts.push_back(get_layout(Const::DESCRITPOR_LAYOUT::UNIFORM_BUFFER));
			for (int i = 0; i < Const::MAX_FRAMES_IN_FLIGHT; i++) {
				uniform_descriptor_sets.push_back(
					Utils::make_buffer_descriptor_set(
						descriptor_pools[i], layouts[3], uniform_buffers[i].descriptor, device
					)
				);
			}
			pipeline_config = make_default_pipeline_config();
			pipeline_config.attribute_descriptions = vertex_builder.build_attribute_descriptions();
			pipeline_config.vertex_binding_descriptions = vertex_builder.build_binding_descriptions();
			pipeline_config.descriptor_set_layouts = layouts;
			pipeline_config.vertex_shader_path = Const::PATH_VERT_SHADERD_DRAW_MODEL_3D;
			pipeline_config.fragment_shader_path = Const::PATH_FRAG_SHADERD_DRAW_MODEL_3D;
			if (Const::ENABLED_TEXTURE_BUCKETS) {
				pipeline_config.fragment_shader_path = Const::PATH_FRAG_SHADERD_DRAW_MODEL_3D_USING_BUCKET;
			}
			pipeline_config.enable_depth_image = true;
			pipeline_config.depth_compare_op = VK_COMPARE_OP_LESS;
			pipeline_config.triangle_trip_order = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			pipeline_config.push_constants_size = sizeof(Push_Constants);

			pipeline.init(pipeline_config);
		}

		std::vector<Draw_Instance_Information> make_model_3D(
			Draw_Information& draw_info, const Model_3D_Attributes& attributes
		) {
			uint32_t model_id = load_model(attributes.path);
			const Model_Information& model = view_model(model_id);
			Draw_Data draw{};
			SSBO_Buffer& ssbo = get_ssbo();
			Storage_Data storage{};
			std::vector<Draw_Instance_Information> draw_instances{};
			storage.transform = Math::make_translation(attributes.position) * Math::make_rotation(attributes.rotation) *
								Math::make_scale(attributes.scale);
			uint32_t storage_id = ssbo.upload_data(&storage, sizeof(Storage_Data), sizeof(Storage_Data));
			Buffer_Range storage_range = ssbo.view_slot(storage_id);
			draw.storage_id = storage_id;
			uint32_t scene_index = model.draw_scene_index;
			uint32_t count = 0;
			for (int i = 0; i < model.meshes.size(); i++) {
				const auto& mesh = model.meshes[i];
				const auto& mesh_transform_offsets = mesh.get_mesh_transform_offsets(scene_index);
				for (int j = 0; j < mesh.primitives.size(); j++) {
					const auto& primitive = mesh.primitives[j];
					const Texture_View& texture_view = view_texture(primitive.texture_id);
					Storage_Texture_Data texture_storage{
						texture_view.bucket_index, texture_view.slot_index, texture_view.tex_scale()
					};
					uint32_t texture_storage_id =
						ssbo.upload_data(&texture_storage, sizeof(Storage_Texture_Data), sizeof(Storage_Texture_Data));
					Buffer_Range texture_storage_range = ssbo.view_slot(texture_storage_id);
					std::vector<Instance_Data> primitive_instances{};
					for (const auto& mesh_tranform_offset : mesh_transform_offsets) {
						Instance_Data primitive_instance_data{
							storage_range.offset_as<Storage_Data>(), mesh_tranform_offset,
							texture_storage_range.offset_as<Storage_Texture_Data>()
						};
						primitive_instances.push_back(primitive_instance_data);
					}
					uint32_t instance_id = ssbo.upload_data(
						primitive_instances.data(), primitive_instances.size() * sizeof(Instance_Data),
						sizeof(Instance_Data)
					);
					Draw_Instance_Information draw_instance{
						instance_id, (uint32_t)primitive_instances.size(), primitive.flat_id
					};
					draw_instances.push_back(draw_instance);
					draw.storage_texture_ids.push_back(texture_storage_id);
					draw.instance_ids.push_back(instance_id);
					count++;
				}
			}
			draw_info.draw_material_id = model_id;
			uint32_t unique_id = unique_id_generator.gen_id();
			draws.insert(unique_id, draw);
			draw_info.unique_id = unique_id;
			return draw_instances;
		}

		void setup_draw(uint32_t frame_index) {
			const Buffer& uniform_buffer = uniform_buffers[frame_index];
			Uniform uniform{};
			glm::mat4 projection = glm::perspective(
				glm::radians(45.0f), (float)swapchain_extent.width / (float)swapchain_extent.height, 0.1f, 100.f
			);
			float radius = 4.0f;
			float angle = (float)glfwGetTime();
			glm::vec3 eye = glm::vec3(radius * sin(angle), 0.0f, radius * cos(angle));
			projection[1][1] *= -1.f;
			glm::mat4 view = glm::lookAt(eye, glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
			uniform.projection = projection;
			uniform.view = view;
			SSBO_Buffer& ssbo = get_ssbo();
			ssbo.update_data(uniform_buffer_ids[frame_index], &uniform);
		}

		void record_upload_uniform_buffer(VkCommandBuffer command_buffer, uint32_t frame_index) {
			uint32_t uniform_range_id = uniform_buffer_ids[frame_index];
			Buffer& uniform_buffer = uniform_buffers[frame_index];
			SSBO_Buffer& ssbo = get_ssbo();
			Buffer_Range range = ssbo.view_slot(uniform_range_id);
			Utils::record_buffer_barrier(
				command_buffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, uniform_buffer.buffer, 0,
				uniform_buffer.size
			);
			{
				VkBufferCopy copy{(VkDeviceSize)range.inner_offset(), 0, (VkDeviceSize)range.inner_size()};
				vkCmdCopyBuffer(command_buffer, ssbo.inner_buffer.buffer, uniform_buffer.buffer, 1, &copy);
			}
			Utils::record_buffer_barrier(
				command_buffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_UNIFORM_READ_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, uniform_buffer.buffer, 0,
				uniform_buffer.size
			);
		}

		void draw(
			VkCommandBuffer command_buffer, uint32_t material_draw_id, uint32_t instance_material_id,
			uint32_t number_instance, uint32_t first_instance_offset, uint32_t frame_index
		) {
			Static_Buffer& static_buffer = get_static_buffer();
			Buffer& instance_buffer = get_instance_buffer(frame_index);
			const Model_Information& model = model_3D_system.view_model(material_draw_id);
			uint32_t number_descriptor_set = 4;
			auto& uniform_descriptor_sets_2 = uniform_descriptor_sets;
			VkDescriptorSet using_descriptor_sets[4] = {
				get_storage_descriptor_set(frame_index), VK_NULL_HANDLE, VK_NULL_HANDLE,
				uniform_descriptor_sets[frame_index]
			};
			VkBuffer binding_buffers[2] = {static_buffer.inner_buffer.buffer, instance_buffer.buffer};
			VkDeviceSize buffer_offsets[2] = {0, 0};
			uint32_t primitive_flat_id = instance_material_id;
			const auto& primitive = model.flat_primitives[primitive_flat_id];
			const Buffer_Range& vertex_range = static_buffer.view_slot_info(primitive.vertex_id);
			const Buffer_Range& indices_range = static_buffer.view_slot_info(primitive.indices_id);
			std::array<VkDescriptorSet, 2> texture_descriptor_sets =
				get_descriptor_set_texture_sampler(primitive.texture_id, frame_index);
			using_descriptor_sets[1] = texture_descriptor_sets[0];
			using_descriptor_sets[2] = texture_descriptor_sets[1];
			bind_draw_resource(
				command_buffer, pipeline.pipeline, pipeline.layout, static_buffer.inner_buffer.buffer, 0,
				VK_INDEX_TYPE_UINT32, 2, binding_buffers, buffer_offsets, number_descriptor_set, using_descriptor_sets
			);
			vkCmdDrawIndexed(
				command_buffer, indices_range.size_as<uint32_t>(), number_instance, indices_range.offset_as<uint32_t>(),
				vertex_range.offset_as<Geometry::Vertex_3D>(), first_instance_offset / (uint32_t)get_instance_size()
			);
		}

		void destroy() {
			pipeline.destroy();
			for (auto& buffer : uniform_buffers) {
				buffer.destroy();
			}
		}

	} // namespace Model_3D

} // namespace Vulkan