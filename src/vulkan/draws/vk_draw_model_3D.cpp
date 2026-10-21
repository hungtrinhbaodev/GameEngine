#include <cmath>
#include <core.h>
#include <geometry_structs.h>
#include <id_generator.h>
#include <math_custom.h>
#include <profiler.h>
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
			uint32_t texture_storage_offset = 0;
			uint32_t mesh_descriptor_offset = 0;
			friend std::ostream& operator<<(std::ostream& os, const Instance_Data& instance) {
				os << "{Model_3D::Instance_Data: storage_offset: " << instance.storage_offset
				   << ", mesh_descriptor_offset: " << instance.mesh_descriptor_offset
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

		struct Texture_Storage_Data {
			int bucket_index = 0;
			int slot_index = 0;
			glm::vec2 tex_scale = {1.f, 1.f};
		};

		struct Skins_Descriptor_Storage_Data {
			uint32_t skins_offset = 0;
		};

		struct Skins_Storage_Information {
			uint32_t storage_id = 0;
		};

		struct Primitive_Storage_Information {
			int primitive_index = -1;
		};

		struct Mesh_Descriptor_Storage_Data {
			uint32_t using_skin = 0;
			uint32_t offset_skins = 0;
			uint32_t offset_transform = 0;
		};

		struct Mesh_Storage_Information {
			uint32_t transform_storage_id = 0;
			Skins_Storage_Information skin_storage{};
			std::vector<Primitive_Storage_Information> primitive_storages{};
		};

		struct Texture_Storage_Information {
			uint32_t texture_storage_id = 0;
		};

		struct Node_Data {
			uint8_t apply_flags = 0;
			glm::mat4 translation{0.f};
			glm::mat4 rotation{1.f};
			glm::mat4 scale{1.f};
			glm::mat4 global_transform{1.f};

			glm::mat4 default_translation{0.f};
			glm::mat4 default_rotation{1.f};
			glm::mat4 default_scale{1.f};
			glm::mat4 default_transform{1.f};

			bool is_use_matrix = false;
			int parent_index;

			void clean() {
				apply_flags = 0;
				global_transform = {1.f};
			}

			void set_translation(const glm::vec3 i_translation) {
				translation = Math::make_translation(i_translation);
				apply_flags |= (1 << 0);
			}

			void set_rotation(const glm::quat i_rotation) {
				rotation = glm::mat4_cast(i_rotation);
				apply_flags |= (1 << 1);
			}

			void set_scale(const glm::vec3 i_scale) {
				scale = Math::make_scale(i_scale);
				apply_flags |= (1 << 2);
			}

			glm::mat4 get_transform() const {
				if (is_use_matrix) {
					return default_transform;
				}
				glm::mat4 o_translation = (apply_flags & (1 << 0)) ? translation : default_translation;
				glm::mat4 o_rotation = (apply_flags & (1 << 1)) ? rotation : default_rotation;
				glm::mat4 o_scale = (apply_flags & (1 << 2)) ? scale : default_scale;
				return o_translation * o_rotation * o_scale;
			}
		};

		struct Model_Data {
			uint32_t model_id = 0;
			int scene_index = -1;
			int animation_index = -1;

			Sparse_Set<uint32_t> node_indices_to_node_data_indices;
			std::vector<Node_Data> nodes;

			Sparse_Set<uint32_t> skins_indices_to_skins_storage_ids;
			Sparse_Set<uint32_t> texture_ids_to_texture_storage_ids;
			std::vector<uint32_t> storage_instance_ids;
			uint32_t mesh_transforms_storage_id = 0;
			uint32_t mesh_descriptors_storage_id = 0;
			uint32_t storage_id = 0;

			std::vector<float> animation_durations;
			std::vector<int> sampler_animation_indices;
			std::vector<glm::mat4> skins_transforms;
			Sparse_Set<std::vector<glm::mat4>> skins_indices_to_skins_transforms;

			bool is_changing(const Model_3D_Attributes& attributes) { return true; }

			void clean() {}

			void update_animation(float dt, SSBO_Buffer& ssbo) {
				if (animation_index < 0)
					return;
				Profiler::start_scope(Const::SCOPE_SAMPLER_ANIMATIONS);
				for (int i = 0; i < nodes.size(); i++) {
					nodes[i].clean();
				}
				const auto& model = view_model(model_id);
				const auto& animation = model.animations[animation_index];
				for (int i = 0; i < animation.channels.size(); i++) {
					animation_durations[i] += dt;
					const auto& channel = animation.channels[i];
					const auto& sampler = animation.samplers[channel.sampler_index];
					const float max_duration = sampler.max_duration();
					if (animation_durations[i] > max_duration) {
						animation_durations[i] -= max_duration;
					}
					const float duration = animation_durations[i];
					uint32_t node_index = node_indices_to_node_data_indices.get(channel.node_index);
					switch (channel.sampler_type) {
						case Model_Sampler_Type::TRANSLATION: {
							glm::vec3 translation = sampler.sampler_vec3(duration, sampler_animation_indices[i]);
							if (channel.use_node_index) {
								nodes[node_index].set_translation(translation);
							}
							break;
						}
						case Model_Sampler_Type::ROTATION: {
							glm::quat rotation = sampler.sampler_quat(duration, sampler_animation_indices[i]);
							if (channel.use_node_index) {
								nodes[node_index].set_rotation(rotation);
							}
							break;
						}
						case Model_Sampler_Type::SCALE: {
							glm::vec3 scale = sampler.sampler_vec3(duration, sampler_animation_indices[i]);
							if (channel.use_node_index) {
								nodes[node_index].set_scale(scale);
							}
							break;
						}
						default: {
							break;
						}
					}
				}
				Profiler::end_scope(Const::SCOPE_SAMPLER_ANIMATIONS);
				Profiler::start_scope(Const::SCOPE_TRAVEL_TREE_NODE_ANIMATIONS);
				for (Node_Data& node : nodes) {
					if (node.parent_index < 0) {
						node.global_transform = node.get_transform();
					} else {
						uint32_t node_parent_index = node_indices_to_node_data_indices.get(node.parent_index);
						const Node_Data& node_parent = nodes[node_parent_index];
						node.global_transform = node_parent.global_transform * node.get_transform();
					}
				}
				Profiler::end_scope(Const::SCOPE_TRAVEL_TREE_NODE_ANIMATIONS);
				Profiler::start_scope(Const::SCOPE_UPLOAD_SKIN_DATA_ANIMATIONS);
				for (uint32_t i = 0; i < model.skins.size(); i++) {
					const auto& skin = model.skins[i];
					auto& skins_transforms = skins_indices_to_skins_transforms.modify(i);
					for (int j = 0; j < skin.joints.size(); j++) {
						int node_index = skin.joints[j];
						const auto& inverse_bind_pose = skin.inverse_bind_poses[j];
						uint32_t node_data_index = node_indices_to_node_data_indices.get(node_index);
						skins_transforms[j] = nodes[node_data_index].global_transform * inverse_bind_pose;
					}
				}
				Profiler::end_scope(Const::SCOPE_UPLOAD_SKIN_DATA_ANIMATIONS);
			};

			void upload_animation_skins(SSBO_Buffer& ssbo) const {
				for (const uint32_t& skin_index : skins_indices_to_skins_storage_ids.keys()) {
					const uint32_t& skins_storage_id = skins_indices_to_skins_storage_ids.get(skin_index);
					const auto& skins_transforms = skins_indices_to_skins_transforms.get(skin_index);
					ssbo.update_data(skins_storage_id, skins_transforms.data());
				}
			};

			std::vector<Draw_Instance_Information> make(const Model_3D_Attributes& attributes, SSBO_Buffer& ssbo) {
				model_id = load_model(attributes.path);
				const auto& model = view_model(model_id);
				scene_index = model.find_scene_index_by(attributes.scene_name);
				animation_index = model.find_animation_index_by(attributes.animation_name);
				/**
				 * @Note: if we can't file the animation,
				 * the mesh transform will be activate
				 * to model place in bind pose.
				 */
				bool has_animation = true;
				if (animation_index >= 0) {
					size_t number_animation = model.animations[animation_index].channels.size();
					animation_durations.resize(number_animation);
					animation_durations.assign(number_animation, 0.f);
					sampler_animation_indices.resize(number_animation);
					sampler_animation_indices.resize(number_animation, -1);
				} else {
					has_animation = false;
				}
				/**
				 * Set up node as scene for tree.
				 */
				model.dfs_read(scene_index, [this](const Model& model, int, int parent_index, int children_index) {
					Node_Data node_data{};
					const auto& node = model.nodes[children_index];
					node_data.parent_index = parent_index;
					node_data.default_transform = node.transform;
					node_data.default_scale = Math::make_scale(node.scale);
					node_data.default_rotation = glm::mat4_cast(node.rotation);
					node_data.default_translation = Math::make_translation(node.translation);
					node_data.is_use_matrix = node.is_use_matrix;
					node_indices_to_node_data_indices.insert(children_index, (uint32_t)nodes.size());
					nodes.push_back(node_data);
				});
				/**
				 * Setup to instance storage data.
				 */
				glm::mat4 instance_transform = attributes.get_transform();
				storage_id = ssbo.upload_data(&instance_transform, sizeof(glm::mat4), sizeof(glm::mat4));
				Buffer_Range storage_range = ssbo.view_slot(storage_id);
				const auto& draw_meshes_info = model.scene_index_to_draw_meshes_info.get(scene_index);
				/**
				 * Setup to texture using by all primitive.
				 */
				for (const auto& primitive : model.primitives) {
					if (!texture_ids_to_texture_storage_ids.has(primitive.texture_id)) {
						const Texture_View& texture_view = view_texture(primitive.texture_id);
						Texture_Storage_Data texture_storage{
							texture_view.bucket_index, texture_view.slot_index, texture_view.tex_scale()
						};
						uint32_t texture_storage_id = ssbo.upload_data(
							&texture_storage, sizeof(Texture_Storage_Data), sizeof(Texture_Storage_Data)
						);
						texture_ids_to_texture_storage_ids.insert(primitive.texture_id, texture_storage_id);
					}
				}
				/**
				 * Setup to skins and mesh transform storage data common that using all mesh.
				 */
				std::vector<glm::mat4> mesh_transform_storages{};
				for (const auto& draw_mesh_info : draw_meshes_info) {
					int skin_index = draw_mesh_info.skin_index;
					if (skin_index >= 0) {
						if (skins_indices_to_skins_storage_ids.has((uint32_t)skin_index)) {
							continue;
						}
						const auto& skin = model.skins[skin_index];
						skins_indices_to_skins_transforms.insert(
							(uint32_t)skin_index, std::vector<glm::mat4>(skin.joints.size(), {1.f})
						);
						const auto& skins_storage_data = skins_indices_to_skins_transforms.get((uint32_t)skin_index);
						uint32_t skins_storage_id = ssbo.upload_data(
							skins_storage_data.data(), skins_storage_data.size() * sizeof(glm::mat4), sizeof(glm::mat4)
						);
						skins_indices_to_skins_storage_ids.insert((uint32_t)skin_index, skins_storage_id);
					}
					mesh_transform_storages.push_back(draw_mesh_info.mesh_transform);
				}
				if (mesh_transform_storages.size() > 0) {
					mesh_transforms_storage_id = ssbo.upload_data(
						mesh_transform_storages.data(), mesh_transform_storages.size() * sizeof(glm::mat4),
						sizeof(glm::mat4)
					);
				}
				/**
				 * Setup to mesh common storage data.
				 */
				Buffer_Range mesh_transform_storage_range = ssbo.view_slot(mesh_transforms_storage_id);
				std::vector<Mesh_Descriptor_Storage_Data> mesh_descriptor_storages(draw_meshes_info.size());
				for (int i = 0; i < draw_meshes_info.size(); i++) {
					const auto& draw_mesh_info = draw_meshes_info[i];
					Mesh_Descriptor_Storage_Data mesh_descriptor_storage_data{};
					Mesh_Storage_Information mesh_storage_info{};
					int skin_index = draw_mesh_info.skin_index;
					mesh_descriptor_storage_data.using_skin = skin_index >= 0 && has_animation;
					/**
					 * Setup storage data for case mesh use skins.
					 */
					if (skin_index >= 0) {
						const uint32_t skins_storage_id = skins_indices_to_skins_storage_ids.get((uint32_t)skin_index);
						const Buffer_Range& skin_range = ssbo.view_slot(skins_storage_id);
						mesh_descriptor_storage_data.offset_skins = skin_range.offset_as<glm::mat4>();
					}
					/**
					 * Setup storage data for case mesh use rigid.
					 */
					mesh_descriptor_storage_data.offset_transform =
						mesh_transform_storage_range.offset_as<glm::mat4>() + i;
					mesh_descriptor_storages[i] = mesh_descriptor_storage_data;
				}
				if (mesh_descriptor_storages.size() > 0) {
					mesh_descriptors_storage_id = ssbo.upload_data(
						mesh_descriptor_storages.data(),
						mesh_descriptor_storages.size() * sizeof(Mesh_Descriptor_Storage_Data),
						sizeof(Mesh_Descriptor_Storage_Data)
					);
				}
				Buffer_Range mesh_descriptors_storage_range = ssbo.view_slot(mesh_descriptors_storage_id);
				/**
				 * Setup to each primitive of each mesh to produce instances data.
				 */
				Sparse_Set<std::vector<Instance_Data>> primitive_indices_to_instances{};
				for (int i = 0; i < draw_meshes_info.size(); i++) {
					const auto& draw_mesh_info = draw_meshes_info[i];
					int mesh_index = draw_mesh_info.mesh_index;
					auto& primitive_indices = model.meshes[mesh_index].primitive_indices;
					for (const auto& primitive_index : primitive_indices) {
						const auto& primitive = model.primitives[primitive_index];
						uint32_t texture_storage_id = texture_ids_to_texture_storage_ids.get(primitive.texture_id);
						const Buffer_Range& texture_storage_range = ssbo.view_slot(texture_storage_id);
						Instance_Data instance_data{
							storage_range.offset_as<glm::mat4>(),
							texture_storage_range.offset_as<Texture_Storage_Data>(),
							mesh_descriptors_storage_range.offset_as<Mesh_Descriptor_Storage_Data>() + i
						};
						if (!primitive_indices_to_instances.has((uint32_t)primitive_index)) {
							primitive_indices_to_instances.insert((uint32_t)primitive_index, {});
						}
						auto& primitive_instances = primitive_indices_to_instances.modify(primitive_index);
						primitive_instances.push_back(instance_data);
					}
				}
				/**
				 * Join all instances by primitive into one vector Draw_Instance_Information.
				 */
				std::vector<Draw_Instance_Information> draw_instances{};
				for (const auto& primitive_index : primitive_indices_to_instances.keys()) {
					const auto& primitive_instances = primitive_indices_to_instances.get(primitive_index);
					uint32_t instances_id = ssbo.upload_data(
						primitive_instances.data(), primitive_instances.size() * sizeof(Instance_Data),
						sizeof(Instance_Data)
					);
					storage_instance_ids.push_back(instances_id);
					Draw_Instance_Information draw_instance{
						instances_id, (uint32_t)primitive_instances.size(), primitive_index
					};
					draw_instances.push_back(draw_instance);
				}
				return draw_instances;
			}
		};

		Id_Generator unique_id_generator{};

		Pipeline_Config pipeline_config{};

		Pipeline pipeline{};

		std::vector<VkDescriptorSetLayout> layouts{};

		/**
		 * @Note: now don't have camera so take the uniform buffer here.
		 */
		std::vector<Buffer> uniform_buffers = {};

		std::vector<uint32_t> uniform_buffer_ids{};

		std::vector<VkDescriptorSet> uniform_descriptor_sets{};

		Sparse_Set<Model_Data> models{};

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
				.add_attribute_description(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Geometry::Vertex_3D, color))
				.add_attribute_description(0, VK_FORMAT_R16G16B16A16_UINT, offsetof(Geometry::Vertex_3D, joints))
				.add_attribute_description(0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Geometry::Vertex_3D, weights));
			vertex_builder.add_binding_description(1, sizeof(Instance_Data), VK_VERTEX_INPUT_RATE_INSTANCE)
				.add_attribute_description(1, VK_FORMAT_R32_UINT, offsetof(Instance_Data, storage_offset))
				.add_attribute_description(1, VK_FORMAT_R32_UINT, offsetof(Instance_Data, texture_storage_offset))
				.add_attribute_description(1, VK_FORMAT_R32_UINT, offsetof(Instance_Data, mesh_descriptor_offset));

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

			pipeline.init(pipeline_config);
		}

		std::vector<Draw_Instance_Information> make_model_3D(
			Draw_Information& draw_info, const Model_3D_Attributes& attributes
		) {
			uint32_t unique_id = unique_id_generator.gen_id();
			Model_Data model_data{};
			auto draw_instances = model_data.make(attributes, get_ssbo());
			models.insert(unique_id, model_data);
			draw_info.sub_unique_id = unique_id;
			draw_info.draw_material_id = model_data.model_id;
			return draw_instances;
		}

		void setup_draw(uint32_t frame_index, float dt) {
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
			Profiler::start_scope(Const::SCOPE_CALCULATE_ANIMATIONS);
			uint32_t number_thread = Core::global_thread_pool->get_thread_ids().size();
			auto& models = Model_3D::models.modify_values();
			if (models.size() <= number_thread * 5) {
				for (auto& model : models) {
					model.update_animation(dt, ssbo);
					model.upload_animation_skins(ssbo);
				}
			} else {
				int number_model_in_thread = (int)std::ceil((float)models.size() / number_thread);
				std::vector<std::future<void>> results{};
				for (int i = 0; i < number_thread; i++) {
					int start = i * number_model_in_thread;
					int end = start + number_model_in_thread;
					auto result = Core::global_thread_pool->enqueue([&models, &ssbo, start, end, dt]() {
						for (int i = start; i < end && i < models.size(); i++) {
							models[i].update_animation(dt, ssbo);
						}
					});
					results.push_back(std::move(result));
				}
				for (auto& result : results) {
					result.get();
				}
				for (const auto& model : models) {
					model.upload_animation_skins(ssbo);
				}
			}
			Profiler::end_scope(Const::SCOPE_CALCULATE_ANIMATIONS);
		}

		void record_upload_uniform_buffer(VkCommandBuffer command_buffer, uint32_t frame_index) {
			uint32_t uniform_range_id = uniform_buffer_ids[frame_index];
			Buffer& uniform_buffer = uniform_buffers[frame_index];
			SSBO_Buffer& ssbo = get_ssbo();
			const Buffer_Range& range = ssbo.view_slot(uniform_range_id);
			Utils::record_buffer_barrier(
				command_buffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, uniform_buffer.buffer, 0,
				uniform_buffer.size
			);
			{
				VkBufferCopy copy{(VkDeviceSize)range.inner_offset(), 0, (VkDeviceSize)range.inner_size()};
				vkCmdCopyBuffer(
					command_buffer, ssbo.staging_buffers[frame_index].buffer, uniform_buffer.buffer, 1, &copy
				);
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
			uint32_t model_id = material_draw_id;
			uint32_t primitive_index = instance_material_id;
			const Model& model = view_model(model_id);
			uint32_t number_descriptor_set = 4;
			VkDescriptorSet using_descriptor_sets[4] = {
				get_storage_descriptor_set(frame_index), VK_NULL_HANDLE, VK_NULL_HANDLE,
				uniform_descriptor_sets[frame_index]
			};
			VkBuffer binding_buffers[2] = {static_buffer.inner_buffer.buffer, instance_buffer.buffer};
			VkDeviceSize buffer_offsets[2] = {0, 0};
			const auto& primitive = model.primitives[primitive_index];
			const Buffer_Range& vertex_range = static_buffer.view_slot(primitive.vertex_id);
			const Buffer_Range& indices_range = static_buffer.view_slot(primitive.indices_id);
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