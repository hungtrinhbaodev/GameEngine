#include <cmath>
#include <consts.h>
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

	namespace Draw_Model_3D {

		bool Model_Helper::has_animation(const std::string& animation_name) const {
			for (const auto& animation : animations) {
				if (animation.animation_name == animation_name) {
					return true;
				}
			}
			return false;
		}

		const Model_Animation_Helper& Model_Helper::get_animation_heler(const std::string& animation_name) const {
			for (const auto& animation : animations) {
				if (animation.animation_name == animation_name) {
					return animation;
				}
			}
			throw std::runtime_error("Fail to get animation in model, please check!");
		}

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

		struct Mesh_Descriptor_Storage_Data {
			uint32_t using_skin = 0;
			uint32_t offset_skins = 0;
			uint32_t offset_transform = 0;
		};

		struct Model_Animation_Data {
			int animation_index = -1;
			std::vector<int> sampler_animation_indices;
			bool is_active = false;
			bool is_loop = false;
			double start_time = 0.f;
			float factor = 0.f;
			float speed = 0.f;
			void clean() {
				animation_index = -1;
				factor = 0.f;
				is_active = false;
				is_loop = false;
				sampler_animation_indices.clear();
			}
			void update_animation_index(int i_animation_index, const Model& model) {
				if (i_animation_index < 0)
					return;
				animation_index = i_animation_index;
				const auto& animation = model.animations[animation_index];
				int number_channel = animation.channels.size();
				sampler_animation_indices.resize(number_channel);
				sampler_animation_indices.assign(number_channel, -1);
			}
		};

		struct Node_Data {
			glm::vec3 final_translation{0.f};
			glm::quat final_rotation{1.f, 0.f, 0.f, 0.f};
			glm::vec3 final_scale{1.f};
			glm::mat4 global_transform{1.f};

			uint8_t apply_flags = 0;
			bool is_use_matrix = false;

			std::array<glm::vec3, Const::MAX_TRACK_MODEL_ANIMATION> track_translations;
			std::array<glm::quat, Const::MAX_TRACK_MODEL_ANIMATION> track_rotations;
			std::array<glm::vec3, Const::MAX_TRACK_MODEL_ANIMATION> track_scales;
			std::array<uint8_t, Const::MAX_TRACK_MODEL_ANIMATION> track_apply_flags;

			/**
			 * @Note: node will be relative with animation
			 * if the node don't relative with animation
			 * we will get it factor = 0 when calculate
			 * transform.
			 */
			std::vector<uint32_t> animation_indices_to_apply_flags;
			std::array<float, Const::MAX_TRACK_MODEL_ANIMATION> translation_factors;
			std::array<float, Const::MAX_TRACK_MODEL_ANIMATION> rotation_factors;
			std::array<float, Const::MAX_TRACK_MODEL_ANIMATION> scale_factors;

			void clean() {
				apply_flags = 0;
				global_transform = {1.f};
				for (int i = 0; i < Const::MAX_TRACK_MODEL_ANIMATION; i++) {
					track_apply_flags[i] = 0;
				}
			}

			void update_apply_flags(const std::vector<Model_Animation>& animations, int node_index) {
				animation_indices_to_apply_flags.clear();
				animation_indices_to_apply_flags.resize(animations.size(), 0);
				for (int i = 0; i < animations.size(); i++) {
					const auto& animation = animations[i];
					const auto& node_indices_to_apply_flags = animation.node_indices_to_apply_flags;
					if (node_indices_to_apply_flags.find(node_index) == node_indices_to_apply_flags.end())
						continue;
					animation_indices_to_apply_flags[i] = node_indices_to_apply_flags.at(node_index);
				}
			};

			void set_translation(const glm::vec3 translation, int track_index) {
				track_apply_flags[track_index] |= (1 << 0);
				track_translations[track_index] = translation;
			}

			void set_rotation(const glm::quat rotation, int track_index) {
				track_apply_flags[track_index] |= (1 << 1);
				track_rotations[track_index] = rotation;
			}

			void set_scale(const glm::vec3 scale, int track_index) {
				track_apply_flags[track_index] |= (1 << 2);
				track_scales[track_index] = scale;
			}

			void update_factors(const std::array<Model_Animation_Data, Const::MAX_TRACK_MODEL_ANIMATION>& animations) {
				for (int i = 0; i < Const::MAX_TRACK_MODEL_ANIMATION; i++) {
					const auto& animation = animations[i];
					if (!animation.is_active)
						continue;
					uint32_t animation_apply_flags = animation_indices_to_apply_flags[animation.animation_index];
					translation_factors[i] = 0.f;
					rotation_factors[i] = 0.f;
					scale_factors[i] = 0.f;
					if (animation_apply_flags > 0) {
						translation_factors[i] = (animation_apply_flags & (1 << 0)) ? animation.factor : 0.f;
						rotation_factors[i] = (animation_apply_flags & (1 << 1)) ? animation.factor : 0.f;
						scale_factors[i] = (animation_apply_flags & (1 << 2)) ? animation.factor : 0.f;
					}
				}
				float sum_factor_translation = 0, sum_factor_rotation = 0, sum_factor_scale = 0;
				for (int i = 0; i < Const::MAX_TRACK_MODEL_ANIMATION; i++) {
					sum_factor_translation += translation_factors[i];
					sum_factor_rotation += rotation_factors[i];
					sum_factor_scale += scale_factors[i];
				}
				for (int i = 0; i < Const::MAX_TRACK_MODEL_ANIMATION; i++) {
					if (sum_factor_translation != 0)
						translation_factors[i] /= sum_factor_translation;
					if (sum_factor_rotation != 0)
						rotation_factors[i] /= sum_factor_rotation;
					if (sum_factor_scale != 0)
						scale_factors[i] /= sum_factor_scale;
				}
			}

			glm::mat4 calculate_transform(
				const Model_Node& node,
				const std::array<Model_Animation_Data, Const::MAX_TRACK_MODEL_ANIMATION>& animations
			) {
				if (is_use_matrix) {
					return node.transform;
				}
				update_factors(animations);
				float sum_factor_translation = 0, sum_factor_rotation = 0, sum_factor_scale = 0;
				for (int i = 0; i < Const::MAX_TRACK_MODEL_ANIMATION; i++) {
					sum_factor_translation += translation_factors[i];
					sum_factor_rotation += rotation_factors[i];
					sum_factor_scale += scale_factors[i];
				}
				float current_factor = 0.f;
				glm::vec3 translation, scale;
				glm::quat rotation;
				float current_translation_factor = 0.f, current_rotation_factor = 0.f, current_scale_factor = 0.f;
				for (int i = 0; i < Const::MAX_TRACK_MODEL_ANIMATION; i++) {
					translation = (track_apply_flags[i] & (1 << 0)) ? track_translations[i] : node.translation;
					rotation = (track_apply_flags[i] & (1 << 1)) ? track_rotations[i] : node.rotation;
					scale = (track_apply_flags[i] & (1 << 2)) ? track_scales[i] : node.scale;
					/**
					 * Handle translation.
					 */
					if (sum_factor_translation <= 0) {
						final_translation = translation;
					} else {
						if (i <= 0 || current_translation_factor + translation_factors[i] <= 0) {
							final_translation = translation;
						} else {
							float translation_factor =
								current_translation_factor / (current_translation_factor + translation_factors[i]);
							final_translation = glm::mix(final_translation, translation, 1.f - translation_factor);
						}
					}
					/**
					 * Handle scale.
					 */
					if (sum_factor_scale <= 0) {
						final_scale = scale;
					} else {
						if (i <= 0 || current_scale_factor + scale_factors[i] <= 0) {
							final_scale = scale;
						} else {
							float scale_factor = current_scale_factor / (current_scale_factor + scale_factors[i]);
							final_scale = glm::mix(final_scale, scale, 1.f - scale_factor);
						}
					}
					/**
					 * Handle rotation.
					 */
					if (sum_factor_rotation <= 0) {
						final_rotation = rotation;
					} else {
						if (i <= 0 || current_rotation_factor + rotation_factors[i] <= 0) {
							final_rotation = rotation;
						} else {
							float rotation_factor =
								current_rotation_factor / (current_rotation_factor + rotation_factors[i]);
							final_rotation = glm::slerp(final_rotation, rotation, 1.f - rotation_factor);
						}
					}
					current_translation_factor += translation_factors[i];
					current_rotation_factor += rotation_factors[i];
					current_scale_factor += scale_factors[i];
				}
				return Math::make_translation(final_translation) * glm::mat4_cast(final_rotation) *
					   Math::make_scale(final_scale);
			}
		};

		struct Model_Data {
			uint32_t model_id = ::Const::MAX_UINT32;
			int scene_index = -1;
			std::vector<Node_Data> nodes;

			Sparse_Set<uint32_t> skins_indices_to_skins_storage_ids;
			Sparse_Set<uint32_t> texture_ids_to_texture_storage_ids;
			uint32_t mesh_transforms_storage_id = ::Const::MAX_UINT32;
			uint32_t mesh_descriptors_storage_id = ::Const::MAX_UINT32;
			uint32_t storage_id = ::Const::MAX_UINT32;

			std::array<Model_Animation_Data, Const::MAX_TRACK_MODEL_ANIMATION> animations;
			Sparse_Set<std::vector<glm::mat4>> skins_indices_to_skins_transforms;

			void clean(SSBO_Buffer& ssbo) const {
				if (storage_id == ::Const::MAX_UINT32)
					return;
				for (const auto& skins_storage_id : skins_indices_to_skins_storage_ids.values()) {
					ssbo.remove_data(skins_storage_id);
				}
				for (const auto& texture_storage_id : texture_ids_to_texture_storage_ids.values()) {
					ssbo.remove_data(texture_storage_id);
				}
				ssbo.remove_data(mesh_transforms_storage_id);
				ssbo.remove_data(mesh_descriptors_storage_id);
				ssbo.remove_data(storage_id);
			}

			void clear(SSBO_Buffer& ssbo, std::vector<Draw_Instance_Information>& draw_instances) {
				clean(ssbo);
				skins_indices_to_skins_storage_ids.clear();
				texture_ids_to_texture_storage_ids.clear();

				nodes.clear();
				for (auto& animation : animations) {
					animation.clean();
				}
				skins_indices_to_skins_transforms.clear();

				mesh_transforms_storage_id = ::Const::MAX_UINT32;
				mesh_descriptors_storage_id = ::Const::MAX_UINT32;
				storage_id = ::Const::MAX_UINT32;

				model_id = ::Const::MAX_UINT32;
				scene_index = -1;
				draw_instances.clear();
			};

			void update_animation_tracks(double current_time) {
				const auto& model = view_model(model_id);
				for (int j = 0; j < Const::MAX_TRACK_MODEL_ANIMATION; j++) {
					auto& animation_data = animations[j];
					if (!animation_data.is_active)
						continue;
					const auto& animation = model.animations[animation_data.animation_index];
					float animation_speed = animation_data.speed;
					float t = (float)(current_time - animation_data.start_time) * animation_speed;
					for (int i = 0; i < animation.channels.size(); i++) {
						const auto& channel = animation.channels[i];
						const auto& sampler = animation.samplers[channel.sampler_index];
						const float max_duration = sampler.max_duration();
						if (t > max_duration && !animation_data.is_loop) {
							animation_data.is_active = false;
						}
						auto& sampler_animation_indices = animation_data.sampler_animation_indices;
						int stride = std::floor(t / max_duration);
						float duration = t - max_duration * stride;
						int node_index = channel.node_index;
						switch (channel.sampler_type) {
							case Model_Sampler_Type::TRANSLATION: {
								glm::vec3 translation = sampler.sampler_vec3(duration, sampler_animation_indices[i]);
								if (node_index >= 0) {
									nodes[node_index].set_translation(translation, j);
								}
								break;
							}
							case Model_Sampler_Type::ROTATION: {
								glm::quat rotation = sampler.sampler_quat(duration, sampler_animation_indices[i]);
								if (node_index >= 0) {
									nodes[node_index].set_rotation(rotation, j);
								}
								break;
							}
							case Model_Sampler_Type::SCALE: {
								glm::vec3 scale = sampler.sampler_vec3(duration, sampler_animation_indices[i]);
								if (node_index >= 0) {
									nodes[node_index].set_scale(scale, j);
								}
								break;
							}
							default: {
								break;
							}
						}
					}
				}
			}

			void update_animation(double current_time) {
				if (is_playing_some_animation()) {
					for (int i = 0; i < nodes.size(); i++) {
						nodes[i].clean();
					}
					update_animation_tracks(current_time);
					const auto& model = view_model(model_id);
					model.dfs_read(scene_index, [this](const Model& model, int, int parent_index, int children_index) {
						const auto& node_config = model.nodes[children_index];
						if (parent_index < 0) {
							auto& child = nodes[children_index];
							child.global_transform = child.calculate_transform(node_config, animations);
						} else {
							if (children_index == 4) {
								int a = 5;
							}
							const auto& parent = nodes[parent_index];
							auto& child = nodes[children_index];
							child.global_transform =
								parent.global_transform * child.calculate_transform(node_config, animations);
						}
					});
					for (uint32_t i = 0; i < model.skins.size(); i++) {
						const auto& skin = model.skins[i];
						auto& skins_transforms = skins_indices_to_skins_transforms.modify(i);
						for (int j = 0; j < skin.joints.size(); j++) {
							int node_index = skin.joints[j];
							const auto& inverse_bind_pose = skin.inverse_bind_poses[j];
							skins_transforms[j] = nodes[node_index].global_transform * inverse_bind_pose;
						}
					}
					for (int i = 0; i < Const::MAX_TRACK_MODEL_ANIMATION; i++) {
						if (!animations[i].is_active) {
							animations[i].clean();
						}
					}
				}
			}

			void upload_animation_skins(SSBO_Buffer& ssbo) const {
				for (const uint32_t& skin_index : skins_indices_to_skins_storage_ids.keys()) {
					const uint32_t& skins_storage_id = skins_indices_to_skins_storage_ids.get(skin_index);
					const auto& skins_transforms = skins_indices_to_skins_transforms.get(skin_index);
					ssbo.update_data(skins_storage_id, skins_transforms.data());
				}
			}

			void clean_animation_when_finish_all(SSBO_Buffer& ssbo) const {
				if (!is_playing_some_animation()) {
					update_using_skin_storage(false, ssbo);
				}
			};

			void update_animation_data(const Model_3D& attributes) {
				uint32_t model_id = load_model(attributes.get_model_path());
				const auto& model = view_model(model_id);
				const auto& animation_attributes = attributes.get_animations();
				const auto& animation_factors = attributes.get_animation_factors();
				for (int i = 0; i < Const::MAX_TRACK_MODEL_ANIMATION; i++) {
					auto& animation = animations[i];
					const auto& animation_attribute = animation_attributes[i];
					if (attributes.is_track_changing(i)) {
						if (animation.is_active) {
							/**
							 * TODO: take a snapshoot here!
							 */
						}
						animation.clean();
						const std::string& animation_name = animation_attribute.animation_name;
						animation.update_animation_index(model.find_animation_index_by(animation_name), model);
						animation.is_active = animation.animation_index >= 0;
						if (animation.animation_index >= 0) {
							animation.is_loop = animation_attribute.is_loop;
							animation.start_time = ::Utils::now();
							animation.factor = animation_factors[i];
							animation.speed = animation_attribute.speed;
						}
					}
				}
			};

			bool is_playing_some_animation() const {
				for (int i = 0; i < Const::MAX_TRACK_MODEL_ANIMATION; i++) {
					if (animations[i].is_active) {
						return true;
					}
				}
				return false;
			};

			/**
			 * @Note: With case the animation change into invalid
			 * or end animation we reset the mesh descriptor storage
			 * to redirect model into bind pose mode.
			 */
			void update_using_skin_storage(uint32_t use_skin, SSBO_Buffer& ssbo) const {
				const Buffer_Range& meshes_descriptor_range = ssbo.view_slot(mesh_descriptors_storage_id);
				uint32_t number_mesh = meshes_descriptor_range.inner_size() / sizeof(Mesh_Descriptor_Storage_Data);
				for (int i = 0; i < number_mesh; i++) {
					ssbo.update_data(
						mesh_descriptors_storage_id, &use_skin,
						meshes_descriptor_range.inner_offset() + sizeof(Mesh_Descriptor_Storage_Data) * i +
							offsetof(Mesh_Descriptor_Storage_Data, using_skin),
						sizeof(uint32_t)
					);
				}
			}

			void update_model_data(
				std::vector<Draw_Instance_Information>& draw_instances, const Model_3D& attributes, SSBO_Buffer& ssbo
			) {
				uint32_t i_model_id = load_model(attributes.get_model_path());
				const auto& model = view_model(i_model_id);
				/**
				 * @Note: if model or scene change we need
				 * recompute all model data to make it right.
				 */
				int i_scene_index = model.find_scene_index_by(attributes.get_scene_name());
				if (model_id != i_model_id || scene_index != i_scene_index) {
					clear(ssbo, draw_instances);
					make(attributes, draw_instances, ssbo);
					update_animation_data(attributes);
					update_using_skin_storage(is_playing_some_animation(), ssbo);
					return;
				}
				update_animation_data(attributes);
				update_using_skin_storage(is_playing_some_animation(), ssbo);
				glm::mat4 instance_transform = attributes.get_transform();
				ssbo.update_data(storage_id, &instance_transform);
			}

			void make(
				const Model_3D& attributes, std::vector<Draw_Instance_Information>& draw_instances, SSBO_Buffer& ssbo
			) {
				model_id = load_model(attributes.get_model_path());
				const auto& model = view_model(model_id);
				scene_index = model.find_scene_index_by(attributes.get_scene_name());
				for (auto& animation : animations) {
					animation.clean();
				}
				/**
				 * Set up node as scene for tree.
				 */
				nodes.resize(model.nodes.size());
				for (int i = 0; i < model.nodes.size(); i++) {
					auto& node_data = nodes[i];
					auto& node = model.nodes[i];
					node_data.is_use_matrix = node.is_use_matrix;
					node_data.update_apply_flags(model.animations, i);
				}
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
					int skin_index = draw_mesh_info.skin_index;
					/**
					 * @Note: using skin will be update in update animation phase.
					 */
					mesh_descriptor_storage_data.using_skin = 0;
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
				for (const auto& primitive_index : primitive_indices_to_instances.keys()) {
					const auto& primitive_instances = primitive_indices_to_instances.get(primitive_index);
					uint32_t instances_id = ssbo.upload_data(
						primitive_instances.data(), primitive_instances.size() * sizeof(Instance_Data),
						sizeof(Instance_Data)
					);
					Draw_Instance_Information draw_instance{
						instances_id, (uint32_t)primitive_instances.size(),
						make_instance_material_id(model_id, primitive_index)
					};
					draw_instances.push_back(draw_instance);
				}
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

		std::unordered_map<std::string, Model_Helper> model_helpers{};

		bool is_material_equal(uint32_t a, uint32_t b) {
			return true;
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

		void init_model(
			Draw_Information& draw_info, std::vector<Draw_Instance_Information>& draw_instances,
			const Model_3D& attributes
		) {
			uint32_t unique_id = unique_id_generator.gen_id();
			Model_Data model_data{};
			model_data.make(attributes, draw_instances, get_ssbo());
			draw_info.sub_unique_id = unique_id;
			draw_info.draw_material_id = model_data.model_id;
			models.insert(unique_id, model_data);
		}

		void update_model(
			Draw_Information& draw_info, std::vector<Draw_Instance_Information>& draw_instances,
			const Model_3D& attributes
		) {
			auto& model = models.modify(draw_info.sub_unique_id);
			model.update_model_data(draw_instances, attributes, get_ssbo());
			draw_info.draw_material_id = model.model_id;
		}

		void remove_model(uint32_t unique_id) {
			if (!models.has(unique_id)) {
				return;
			}
			const auto& model_data = models.get(unique_id);
			model_data.clean(get_ssbo());
			models.erase(unique_id);
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
			auto& models = Draw_Model_3D::models.modify_values();
			double current_time = ::Utils::now();
			if (models.size() <= number_thread * 5) {
				for (auto& model : models) {
					model.update_animation(current_time);
					model.upload_animation_skins(ssbo);
					model.clean_animation_when_finish_all(ssbo);
				}
			} else {
				int number_model_in_thread = (int)std::ceil((float)models.size() / number_thread);
				std::vector<std::future<void>> results{};
				for (int i = 0; i < number_thread; i++) {
					int start = i * number_model_in_thread;
					int end = start + number_model_in_thread;
					auto result = Core::global_thread_pool->enqueue([&models, start, end, current_time]() {
						for (int i = start; i < end && i < models.size(); i++) {
							models[i].update_animation(current_time);
						}
					});
					results.push_back(std::move(result));
				}
				for (int i = 0; i < results.size(); i++) {
					results[i].get();
					int start = i * number_model_in_thread;
					int end = start + number_model_in_thread;
					for (int j = start; j < end && j < models.size(); j++) {
						models[j].upload_animation_skins(ssbo);
						models[j].clean_animation_when_finish_all(ssbo);
					}
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

		uint32_t make_instance_material_id(uint32_t model_id, uint32_t primitive_index) {
			uint32_t result = 0;
			result |= (model_id << 16);
			result |= primitive_index;
			return result;
		}

		void resovle_instance_material_id(
			uint32_t instance_material_id, uint32_t& model_id, uint32_t& primitive_index
		) {
			uint16_t result_primitive_index = (uint16_t)instance_material_id;
			uint16_t result_model_id = instance_material_id >> 16;
			model_id = (uint32_t)result_model_id;
			primitive_index = (uint32_t)result_primitive_index;
		}

		void draw(
			VkCommandBuffer command_buffer, uint32_t material_draw_id, uint32_t instance_material_id,
			uint32_t number_instance, uint32_t first_instance_offset, uint32_t frame_index
		) {
			Static_Buffer& static_buffer = get_static_buffer();
			Buffer& instance_buffer = get_instance_buffer(frame_index);
			uint32_t model_id = 0;
			uint32_t primitive_index = 0;
			resovle_instance_material_id(instance_material_id, model_id, primitive_index);
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

		const Model_Helper& get_model_helper(const std::string& model_path) {
			if (model_helpers.find(model_path) == model_helpers.end()) {
				make_model_helper(model_path);
			}
			return model_helpers[model_path];
		}

		void make_model_helper(const std::string& model_path) {
			if (model_helpers.find(model_path) != model_helpers.end()) {
				return;
			}
			uint32_t model_id = load_model(model_path);
			const Model& model = view_model(model_id);
			Model_Helper helper{};
			for (const auto& scene : model.scenes) {
				helper.scene_names.push_back(scene.name);
			}
			for (const auto& model_animation : model.animations) {
				Model_Animation_Helper animation_helper{};
				animation_helper.animation_name = model_animation.name;
				animation_helper.duration = model_animation.get_animation_duration();
				helper.animations.push_back(animation_helper);
			}
			model_helpers[model_path] = helper;
		}

	} // namespace Draw_Model_3D

} // namespace Vulkan