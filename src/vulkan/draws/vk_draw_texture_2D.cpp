#include <id_generator.h>
#include <map>
#include <utils.h>
#include <vulkan/draws/vk_draw_texture_2D.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_descriptor.h>
#include <vulkan/vk_pipeline.h>
#include <vulkan/vk_structs.h>
#include <vulkan/vk_utils.h>
#include <vulkan/vk_vertex_input_builder.h>

namespace Vulkan {

	namespace Texture_2D {

		struct Instance_Data {
			glm::vec2 tex_size{0.f, 0.f};
			glm::vec2 translation{0.f, 0.f};
			glm::vec2 scale{1.f, 1.f};
			glm::vec2 anchor{0.f, 0.f};
			glm::vec4 rect{0.f, 0.f, 0.f, 0.f};
			float rotation = 0.f;
			uint32_t bucket_index = 0;
			uint32_t slot_index = 0;
			void make(Texture_2D_Attributes attributes, Texture_View texture_view) {
				tex_size = glm::vec2(
					texture_view.width * attributes.texture_rect.ratio_width,
					texture_view.height * attributes.texture_rect.ratio_height
				);
				translation = attributes.position;
				scale = attributes.scale;
				anchor = attributes.anchor;
				rect = attributes.texture_rect.to_vec4();
				bucket_index = texture_view.bucket_index;
				slot_index = texture_view.slot_index;
			}
			friend std::ostream& operator<<(std::ostream& os, const Instance_Data& instance) {
				os << "{Rectangle::Instance_Data: translation: " << instance.translation
				   << ",  tex_size: " << instance.tex_size << ", anchor: " << instance.anchor
				   << ", rect: " << instance.rect << ", rotation: " << instance.rotation
				   << ", scale: " << instance.scale << ", bucket_index: " << instance.bucket_index
				   << ", slot_index: " << instance.slot_index << "}";
				return os;
			}
		};

		struct Push_Constants {
			glm::vec2 screen_size{0.f, 0.f};
			uint32_t vertex_offset = 0;
		};

		struct Materital_Data {
			Const::TEXTURE_STORAGE_MODE storage_mode = Const::TEXTURE_STORAGE_MODE::INDIVIDUAL;
			uint32_t texture_id = 0;
		};

		std::vector<Geometry::Vertex_2D> TEXTURE_VERTICES = {{{0.f, 0.f}}, {{0.f, 1.f}}, {{1.f, 1.f}}, {{1.f, 0.f}}};

		std::vector<uint16_t> TEXTURE_INDICES = {0, 1, 2, 2, 3, 0};

		Pipeline_Config pipeline_config{};

		Pipeline pipeline{};

		std::vector<VkDescriptorSetLayout> layouts{};

		std::unordered_map<uint32_t, std::vector<VkDescriptorSet>> texture_descriptor_sets{};

		Id_Generator material_id_generator{};

		std::unordered_map<uint32_t, Materital_Data> material_by_ids;

		std::vector<VkDescriptorSet> texture_bucket_descriptor_sets{};

		std::vector<VkDescriptorSet> default_texture_descriptor_sets{};

		uint32_t vertex_id = 0;

		uint32_t indices_id = 0;

		bool is_material_equal(uint32_t a, uint32_t b) {
			return a == b;
		}

		void init() {
			Vertex_Input_Builder vertex_builder{};
			vertex_builder.add_binding_description(0, sizeof(Geometry::Vertex_2D), VK_VERTEX_INPUT_RATE_VERTEX)
				.add_attribute_description(0, VK_FORMAT_R32G32_SFLOAT, offsetof(Geometry::Vertex_2D, position));
			vertex_builder.add_binding_description(1, sizeof(Instance_Data), VK_VERTEX_INPUT_RATE_INSTANCE)
				.add_attribute_description(1, VK_FORMAT_R32G32_SFLOAT, offsetof(Instance_Data, tex_size))
				.add_attribute_description(1, VK_FORMAT_R32G32_SFLOAT, offsetof(Instance_Data, translation))
				.add_attribute_description(1, VK_FORMAT_R32G32_SFLOAT, offsetof(Instance_Data, scale))
				.add_attribute_description(1, VK_FORMAT_R32G32_SFLOAT, offsetof(Instance_Data, anchor))
				.add_attribute_description(1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Instance_Data, rect))
				.add_attribute_description(1, VK_FORMAT_R32_SFLOAT, offsetof(Instance_Data, rotation))
				.add_attribute_description(1, VK_FORMAT_R32_SINT, offsetof(Instance_Data, bucket_index))
				.add_attribute_description(1, VK_FORMAT_R32_SINT, offsetof(Instance_Data, slot_index));

			Descriptor_Set_Layout_Builder layout_builder{};
			layout_builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
			layouts.push_back(layout_builder.build(Vulkan::device));
			if (Const::ENABLED_TEXTURE_BUCKETS) {
				layouts.push_back(Vulkan::texture_system.make_bucket_descriptor_set_layout());
				texture_bucket_descriptor_sets = Vulkan::texture_system.make_bucket_descriptor_sets(layouts[1]);
				default_texture_descriptor_sets =
					Vulkan::texture_system.make_default_texture_descriptor_sets(layouts[0]);
			}

			pipeline_config = Vulkan::make_default_pipeline_config();
			pipeline_config.attribute_descriptions = vertex_builder.build_attribute_descriptions();
			pipeline_config.vertex_binding_descriptions = vertex_builder.build_binding_descriptions();
			pipeline_config.descriptor_set_layouts = layouts;
			pipeline_config.vertex_shader_path = Const::PATH_VERT_SHADERD_DRAW_TEXTURE_2D;
			pipeline_config.fragment_shader_path = Const::PATH_FRAG_SHADERD_DRAW_TEXTURE_2D;
			if (Const::ENABLED_TEXTURE_BUCKETS) {
				pipeline_config.fragment_shader_path = Const::PATH_FRAG_SHADERD_DRAW_TEXTURE_2D_USING_BUCKET;
			}
			pipeline.init(pipeline_config);

			Static_Buffer& static_buffer = get_static_buffer();
			vertex_id = static_buffer.upload_data(
				TEXTURE_VERTICES.data(), sizeof(Geometry::Vertex_2D) * TEXTURE_VERTICES.size(),
				sizeof(Geometry::Vertex_2D)
			);
			indices_id = static_buffer.upload_data(
				TEXTURE_INDICES.data(), sizeof(uint16_t) * TEXTURE_INDICES.size(), sizeof(uint16_t)
			);
		}

		size_t get_instance_size() {
			return sizeof(Instance_Data);
		}

		Draw_Information make_texture_2D(const Texture_2D_Attributes& texture_attributes) {
			uint32_t texture_id =
				Vulkan::texture_system.load_texture(texture_attributes.path, Const::ENABLED_TEXTURE_BUCKETS);
			Texture_View texture_view = Vulkan::texture_system.view_texture(texture_id);
			if (texture_descriptor_sets.find(texture_id) == texture_descriptor_sets.end() &&
				texture_view.storage_mode == Const::TEXTURE_STORAGE_MODE::INDIVIDUAL) {
				texture_descriptor_sets[texture_id] = Utils::make_texture_descriptor_sets(
					descriptor_pools, layouts[0], texture_view.image.get_descriptor_info(texture_view.slot_index),
					device
				);
			}
			uint32_t draw_material_id = material_id_generator.gen_id();
			Draw_Information draw_info{Const::DRAW_ID::DRAW_TEXTURE_2D};
			draw_info.draw_material_id = draw_material_id;
			material_by_ids[draw_material_id] = {texture_view.storage_mode, texture_id};
			Instance_Data instance{};
			instance.make(texture_attributes, texture_view);
			SSBO_Buffer& ssbo = get_ssbo();
			uint32_t instance_id = ssbo.upload_data(&instance, sizeof(Instance_Data));
			draw_info.instance_id = instance_id;
			return draw_info;
		}

		void update_texture_2D(const Draw_Information draw_info, const Texture_2D_Attributes& texture_attributes) {
			if (material_by_ids.find(draw_info.draw_material_id) == material_by_ids.end()) {
				return;
			}
			Materital_Data& material = material_by_ids[draw_info.draw_material_id];
			uint32_t texture_id = Vulkan::texture_system.load_texture(texture_attributes.path);
			Texture_View texture_view = Vulkan::texture_system.view_texture(texture_id);
			material.storage_mode = texture_view.storage_mode;
			material.texture_id = texture_id;
			Instance_Data instance{};
			instance.make(texture_attributes, texture_view);
			SSBO_Buffer& ssbo = get_ssbo();
			ssbo.update_data(draw_info.instance_id, &instance);
		}

		void draw(
			VkCommandBuffer command_buffer, uint32_t material_draw_id, uint32_t number_instance,
			uint32_t first_instance_offset, uint32_t frame_index
		) {
			Static_Buffer& static_buffer = get_static_buffer();
			Static_Buffer_Range indices_range = static_buffer.view_slot_info(indices_id);
			Static_Buffer_Range vertex_range = static_buffer.view_slot_info(vertex_id);
			Buffer instance_buffer = get_instance_buffer();
			Materital_Data material = material_by_ids[material_draw_id];
			VkDescriptorSet* descriptor_sets = nullptr;
			uint32_t number_descriptor_set = 1;
			if (Const::ENABLED_TEXTURE_BUCKETS) {
				VkDescriptorSet bind_descriptor_sets[2] = {};
				if (material.storage_mode == Const::TEXTURE_STORAGE_MODE::BUCKET) {
					bind_descriptor_sets[0] = default_texture_descriptor_sets[frame_index];
				} else {
					VkDescriptorSet texture_descriptor_set = texture_descriptor_sets[material.texture_id][frame_index];
					bind_descriptor_sets[0] = texture_descriptor_set;
				}
				bind_descriptor_sets[1] = texture_bucket_descriptor_sets[frame_index];
				number_descriptor_set = 2;
				descriptor_sets = bind_descriptor_sets;
			} else {
				VkDescriptorSet bind_descriptor_sets[1] = {};
				VkDescriptorSet texture_descriptor_set = texture_descriptor_sets[material.texture_id][frame_index];
				bind_descriptor_sets[0] = texture_descriptor_set;
				number_descriptor_set = 1;
				descriptor_sets = bind_descriptor_sets;
			}
			VkBuffer binding_buffers[2] = {static_buffer.inner_buffer.buffer, instance_buffer.buffer};
			VkDeviceSize buffer_offsets[2] = {0, 0};
			bind_draw_resource(
				command_buffer, pipeline.pipeline, pipeline.layout, static_buffer.inner_buffer.buffer, 0,
				VK_INDEX_TYPE_UINT16, 2, binding_buffers, buffer_offsets, number_descriptor_set, descriptor_sets
			);
			Push_Constants constants{Utils::get_window_size(device), vertex_range.offset_as<Geometry::Vertex_2D>()};
			vkCmdPushConstants(
				command_buffer, pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
				sizeof(Push_Constants), &constants
			);
			vkCmdDrawIndexed(
				command_buffer, indices_range.size_as<uint16_t>(), number_instance, indices_range.offset_as<uint16_t>(),
				vertex_range.offset_as<Geometry::Vertex_2D>(), first_instance_offset / (uint32_t)(get_instance_size())
			);
		}

		void destroy() {
			for (const VkDescriptorSetLayout layout : layouts) {
				vkDestroyDescriptorSetLayout(Vulkan::device, layout, nullptr);
			}
			pipeline.destroy();
		}

	} // namespace Texture_2D

} // namespace Vulkan