#include <geometry_structs.h>
#include <math_custom.h>
#include <sparse_set.h>
#include <vulkan/draws/vk_draw_model_3D.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_descriptor.h>
#include <vulkan/vk_pipeline.h>
#include <vulkan/vk_uniform.h>
#include <vulkan/vk_utils.h>
#include <vulkan/vk_vertex_input_builder.h>

namespace Vulkan {

	namespace Model_3D {

		struct Instance_Data {
			glm::mat4 transform{1.f};
			friend std::ostream& operator<<(std::ostream& os, const Instance_Data& instance) {
				os << "{Model_3D::Instance_Data: transform: " << instance.transform << "}";
				return os;
			}
		};

		struct Push_Constants {
			glm::mat4 mesh_transform{1.f};
			int bucket_index = 0;
			int slot_index = 0;
		};

		struct Material_Data {
			uint32_t model_id = 0;
		};

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
				.add_mat4_attribute_description(1, 0);

			Descriptor_Set_Layout_Builder layout_builder{};
			layout_builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
			layouts.push_back(layout_builder.build(Vulkan::device));
			default_texture_descriptor_sets = texture_system.make_default_texture_descriptor_sets(layouts[0]);
			if (Const::ENABLED_TEXTURE_BUCKETS) {
				layouts.push_back(texture_system.make_bucket_descriptor_set_layout());
				texture_bucket_descriptor_sets = texture_system.make_bucket_descriptor_sets(layouts[1]);
			} else {
				layout_builder.clear();
				layouts.push_back(layout_builder.build(device));
			}
			layout_builder.clear();
			layout_builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
			layouts.push_back(layout_builder.build(device));
			for (int i = 0; i < Const::MAX_FRAMES_IN_FLIGHT; i++) {
				uniform_descriptor_sets.push_back(
					Utils::make_buffer_descriptor_set(
						descriptor_pools[i], layouts[2], uniform_buffers[i].descriptor, device
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

		Draw_Information make_model_3D(const Model_3D_Attributes& attributes) {
			uint32_t model_id = model_3D_system.load_model(attributes.path);
			Model_Information model = model_3D_system.view_model(model_id);
			for (const auto& mesh : model.meshes) {
				for (auto const& primitive : mesh.primitives) {
					Texture_View texture_view = texture_system.view_texture(primitive.texture_id);
					if (texture_view.storage_mode == Const::TEXTURE_STORAGE_MODE::INDIVIDUAL &&
						!texture_descriptor_sets.has(primitive.texture_id)) {
						texture_descriptor_sets.insert(
							primitive.texture_id,
							Utils::make_texture_descriptor_sets(
								descriptor_pools, layouts[0],
								texture_view.image.get_descriptor_info(texture_view.slot_index), device
							)
						);
					}
				}
			}
			Instance_Data instance{};
			instance.transform = Math::make_translation(attributes.position) *
								 Math::make_rotation(attributes.rotation) * Math::make_scale(attributes.scale);
			SSBO_Buffer& ssbo = get_ssbo();
			Draw_Information draw_info{Const::DRAW_ID::DRAW_MODEL_3D};
			draw_info.draw_material_id = model_id;
			draw_info.instance_id = ssbo.upload_data(&instance, sizeof(Instance_Data));
			return draw_info;
		}

		void setup_draw(uint32_t frame_index) {
			const Buffer& uniform_buffer = uniform_buffers[frame_index];
			Uniform uniform{};
			glm::mat4 projection = glm::perspective(
				glm::radians(45.0f), (float)swapchain_extent.width / (float)swapchain_extent.height, 0.1f, 100.f
			);
			float radius = 3.0f;
			float angle = (float)glfwGetTime();
			glm::vec3 eye = glm::vec3(radius * sin(angle), 0.0f, radius * cos(angle));
			projection[1][1] *= -1.f;
			glm::mat4 view = glm::lookAt(eye, glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
			uniform.projection = projection;
			uniform.view = view;
			SSBO_Buffer& ssbo = get_ssbo();
			ssbo.update_data(uniform_buffer_ids[frame_index], &uniform);
			ssbo.transfer_data_to(uniform_buffer_ids[frame_index], uniform_buffer.buffer, 0);
		}

		void draw(
			VkCommandBuffer command_buffer, uint32_t material_draw_id, uint32_t number_instance,
			uint32_t first_instance_offset, uint32_t frame_index
		) {
			Static_Buffer& static_buffer = get_static_buffer();
			Buffer& instance_buffer = get_instance_buffer();
			const Model_Information& model = model_3D_system.view_model(material_draw_id);
			uint32_t number_descriptor_set = 3;
			VkDescriptorSet using_descriptor_sets[3] = {
				VK_NULL_HANDLE, VK_NULL_HANDLE, uniform_descriptor_sets[frame_index]
			};
			if (Const::ENABLED_TEXTURE_BUCKETS) {
				using_descriptor_sets[1] = texture_bucket_descriptor_sets[frame_index];
			}
			VkBuffer binding_buffers[2] = {static_buffer.inner_buffer.buffer, instance_buffer.buffer};
			VkDeviceSize buffer_offsets[2] = {0, 0};
			uint32_t draw_scene_index = model.draw_scene_index;
			for (int i = 0; i < model.meshes.size(); i++) {
				const auto& mesh = model.meshes[i];
				const std::vector<glm::mat4>& mesh_transforms =
					model.global_meshes_transform.at(draw_scene_index).at(i);
				Push_Constants push_constants{};
				glm::vec3 mesh_origin = mesh.get_mesh_origin();
				for (const auto& transform : mesh_transforms) {
					push_constants.mesh_transform = transform * Math::make_translation(-mesh_origin);
					vkCmdPushConstants(
						command_buffer, pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
						sizeof(Push_Constants), &push_constants
					);
					for (const auto& primitive : mesh.primitives) {
						Texture_View texture_view = model_3D_system.texture_system->view_texture(primitive.texture_id);
						int bucket_index = -1;
						uint32_t slot_index = 0;
						if (texture_view.storage_mode == Const::TEXTURE_STORAGE_MODE::INDIVIDUAL) {
							using_descriptor_sets[0] = texture_descriptor_sets.get(primitive.texture_id)[frame_index];
						} else {
							bucket_index = texture_view.bucket_index;
							slot_index = texture_view.slot_index;
						}
						vkCmdPushConstants(
							command_buffer, pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
							offsetof(Push_Constants, bucket_index), sizeof(int), &bucket_index
						);
						vkCmdPushConstants(
							command_buffer, pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
							offsetof(Push_Constants, slot_index), sizeof(int), &slot_index
						);
						Static_Buffer_Range vertex_range = static_buffer.view_slot_info(primitive.vertex_id);
						Static_Buffer_Range indices_range = static_buffer.view_slot_info(primitive.indices_id);
						bind_draw_resource(
							command_buffer, pipeline.pipeline, pipeline.layout, static_buffer.inner_buffer.buffer, 0,
							VK_INDEX_TYPE_UINT32, 2, binding_buffers, buffer_offsets, number_descriptor_set,
							using_descriptor_sets
						);
						vkCmdDrawIndexed(
							command_buffer, indices_range.size_as<uint32_t>(), number_instance,
							indices_range.offset_as<uint32_t>(), vertex_range.offset_as<Geometry::Vertex_3D>(),
							first_instance_offset / (uint32_t)get_instance_size()
						);
					}
				}
			}
		}

		void destroy() {
			for (const VkDescriptorSetLayout& layout : layouts) {
				vkDestroyDescriptorSetLayout(device, layout, nullptr);
			}
			pipeline.destroy();
			for (auto& buffer : uniform_buffers) {
				buffer.destroy();
			}
		}

	} // namespace Model_3D

} // namespace Vulkan