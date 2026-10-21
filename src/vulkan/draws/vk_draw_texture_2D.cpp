#include <id_generator.h>
#include <map>
#include <utils.h>
#include <vulkan/draws/vk_draw_texture_2D.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_descriptor.h>
#include <vulkan/vk_draw.h>
#include <vulkan/vk_pipeline.h>
#include <vulkan/vk_structs.h>
#include <vulkan/vk_utils.h>
#include <vulkan/vk_vertex_input_builder.h>

namespace Vulkan {

	namespace Texture_2D {

		struct Instance_Data {
			uint32_t storage_offset = 0;
			uint32_t vertex_offset = 0;
			uint32_t config_draw_offset = 0;
		};

		struct Storage_Data {
			glm::vec2 tex_size{0.f, 0.f};		// 8
			glm::vec2 translation{0.f, 0.f};	// 8
			glm::vec2 scale{1.f, 1.f};			// 8
			glm::vec2 anchor{0.f, 0.f};			// 8
			glm::vec4 rect{0.f, 0.f, 0.f, 0.f}; // 16
			float rotation = 0.f;				// 4
			int32_t bucket_index = 0;			// 4
			int32_t slot_index = 0;				// 4
			float _pad = 0.f;					// 4
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
			friend std::ostream& operator<<(std::ostream& os, const Storage_Data& instance) {
				os << "{Rectangle::Instance_Data: translation: " << instance.translation
				   << ",  tex_size: " << instance.tex_size << ", anchor: " << instance.anchor
				   << ", rect: " << instance.rect << ", rotation: " << instance.rotation
				   << ", scale: " << instance.scale << ", bucket_index: " << instance.bucket_index
				   << ", slot_index: " << instance.slot_index << "}";
				return os;
			}
		};

		struct Draw_Data {
			uint32_t storage_id = 0;
		};

		std::vector<Geometry::Vertex_2D> TEXTURE_VERTICES = {{{0.f, 0.f}}, {{0.f, 1.f}}, {{1.f, 1.f}}, {{1.f, 0.f}}};

		std::vector<uint16_t> TEXTURE_INDICES = {0, 1, 2, 2, 3, 0};

		Pipeline_Config pipeline_config{};

		Pipeline pipeline{};

		std::vector<VkDescriptorSetLayout> layouts{};

		Sparse_Set<Draw_Data> draws{};

		uint32_t vertex_id = 0;

		uint32_t indices_id = 0;

		bool is_material_equal(uint32_t a, uint32_t b) {
			if (a == b)
				return true;
			const Texture_View& texture_view_a = view_texture(a);
			const Texture_View& texture_view_b = view_texture(b);
			return texture_view_a.storage_mode == Const::TEXTURE_STORAGE_MODE::BUCKET &&
				   texture_view_b.storage_mode == Const::TEXTURE_STORAGE_MODE::BUCKET;
		}

		void init() {
			Vertex_Input_Builder vertex_builder{};
			vertex_builder.add_binding_description(0, sizeof(Geometry::Vertex_2D), VK_VERTEX_INPUT_RATE_VERTEX)
				.add_attribute_description(0, VK_FORMAT_R32G32_SFLOAT, offsetof(Geometry::Vertex_2D, position));
			vertex_builder.add_binding_description(1, sizeof(Instance_Data), VK_VERTEX_INPUT_RATE_INSTANCE)
				.add_attribute_description(1, VK_FORMAT_R32_UINT, offsetof(Instance_Data, storage_offset))
				.add_attribute_description(1, VK_FORMAT_R32_UINT, offsetof(Instance_Data, vertex_offset))
				.add_attribute_description(1, VK_FORMAT_R32_UINT, offsetof(Instance_Data, config_draw_offset));

			layouts.push_back(get_layout(Const::DESCRITPOR_LAYOUT::STORAGE_BUFFER));
			layouts.push_back(get_layout(Const::DESCRITPOR_LAYOUT::IMAGE_SAMPLER));
			if (Const::ENABLED_TEXTURE_BUCKETS) {
				layouts.push_back(get_layout(Const::DESCRITPOR_LAYOUT::BUCKET_IMAGE_SAMPLER));
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

		std::vector<Draw_Instance_Information> make_texture_2D(
			Draw_Information& draw_info, const Texture_2D_Attributes& texture_attributes
		) {
			uint32_t texture_id = load_texture(texture_attributes.path);
			Texture_View texture_view = view_texture(texture_id);
			Storage_Data storage{};
			storage.make(texture_attributes, texture_view);
			SSBO_Buffer& ssbo = get_ssbo();
			uint32_t storage_id = ssbo.upload_data(&storage, sizeof(Storage_Data), sizeof(Storage_Data));
			Buffer_Range storage_range = ssbo.view_slot(storage_id);
			Static_Buffer& static_buffer = get_static_buffer();
			const Buffer_Range& vertex_range = static_buffer.view_slot(vertex_id);
			uint32_t config_draw_offset = Utils::get_draw_2D_config_data_offset(device);
			Instance_Data instance{
				storage_range.offset_as<Storage_Data>(), vertex_range.offset_as<Geometry::Vertex_2D>(),
				config_draw_offset
			};
			uint32_t instance_id = ssbo.upload_data(&instance, sizeof(Instance_Data));
			draws.insert(instance_id, {storage_id});
			draw_info.draw_material_id = texture_id;
			draw_info.sub_unique_id = instance_id;
			return {{instance_id}};
		}

		void update_texture_2D(Draw_Information& draw_info, const Texture_2D_Attributes& texture_attributes) {
			uint32_t texture_id = load_texture(texture_attributes.path);
			Texture_View texture_view = view_texture(texture_id);
			draw_info.draw_material_id = texture_id;
			Storage_Data storage{};
			storage.make(texture_attributes, texture_view);
			SSBO_Buffer& ssbo = get_ssbo();
			const Draw_Data& data = draws.get(draw_info.sub_unique_id);
			ssbo.update_data(data.storage_id, &storage);
		}

		void draw(
			VkCommandBuffer command_buffer, uint32_t material_draw_id, uint32_t number_instance,
			uint32_t first_instance_offset, uint32_t frame_index
		) {
			Static_Buffer& static_buffer = get_static_buffer();
			Buffer_Range indices_range = static_buffer.view_slot(indices_id);
			Buffer_Range vertex_range = static_buffer.view_slot(vertex_id);
			Buffer instance_buffer = get_instance_buffer(frame_index);
			uint32_t texture_id = material_draw_id;
			VkDescriptorSet storage_descriptor_set = get_storage_descriptor_set(frame_index);
			auto texture_descriptor = get_descriptor_set_texture_sampler(texture_id, frame_index);
			VkDescriptorSet bind_descriptor_sets[3] = {
				storage_descriptor_set, texture_descriptor[0], texture_descriptor[1]
			};
			uint32_t number_descriptor_set = 0;
			if (Const::ENABLED_TEXTURE_BUCKETS) {
				number_descriptor_set = 3;
			} else {
				number_descriptor_set = 2;
			}
			VkBuffer binding_buffers[2] = {static_buffer.inner_buffer.buffer, instance_buffer.buffer};
			VkDeviceSize buffer_offsets[2] = {0, 0};
			bind_draw_resource(
				command_buffer, pipeline.pipeline, pipeline.layout, static_buffer.inner_buffer.buffer, 0,
				VK_INDEX_TYPE_UINT16, 2, binding_buffers, buffer_offsets, number_descriptor_set, bind_descriptor_sets
			);
			vkCmdDrawIndexed(
				command_buffer, indices_range.size_as<uint16_t>(), number_instance, indices_range.offset_as<uint16_t>(),
				vertex_range.offset_as<Geometry::Vertex_2D>(), first_instance_offset / (uint32_t)(get_instance_size())
			);
		}

		void destroy() {
			pipeline.destroy();
		}

	} // namespace Texture_2D

} // namespace Vulkan