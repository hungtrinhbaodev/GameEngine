#include <geometry_structs.h>
#include <log.h>
#include <utils.h>
#include <vulkan/draws/vk_draw_rectangle.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_draw.h>
#include <vulkan/vk_pipeline.h>
#include <vulkan/vk_utils.h>
#include <vulkan/vk_vertex_input_builder.h>

namespace Vulkan {

	namespace Rectangle {

		struct Instance_Data {
			uint32_t storage_offset = 0;
			uint32_t config_offset = 0;
		};

		struct Storage_Data {
			glm::vec2 translation{0.f, 0.f}; // 8
			glm::vec2 size{0.f, 0.f};		 // 8
			glm::vec3 color{1.f, 1.f, 1.f};	 // 12
			float rotation = 0.f;			 // 4
			glm::vec2 anchor{0.f, 0.f};		 // 8
			float _pad[2];					 // 8
			void make(const Rectangle_Attributes& attributes) {
				translation = attributes.position;
				size = glm::vec2(attributes.width, attributes.height);
				anchor = attributes.anchor;
				color = attributes.color;
				rotation = attributes.rotation;
			}
			friend std::ostream& operator<<(std::ostream& os, const Storage_Data& instance) {
				os << "{Rectangle::Instance_Data: translation: " << instance.translation
				   << ",  scale: " << instance.size << ", anchor: " << instance.anchor << ", color: " << instance.color
				   << ", rotation: " << instance.rotation << "}";
				return os;
			}
		};

		struct Draw_Data {
			uint32_t storage_id = 0;
		};

		std::vector<Geometry::Vertex_2D> RECTANGLE_VERTICES = {{{0.f, 0.f}}, {{0.f, 1.f}}, {{1.f, 1.f}}, {{1.f, 0.f}}};

		std::vector<uint16_t> RECTANGLE_INDICES = {0, 1, 2, 2, 3, 0};

		uint32_t vertex_id = 0;

		uint32_t indices_id = 0;

		Pipeline_Config pipeline_config{};

		Pipeline pipeline{};

		std::vector<VkDescriptorSetLayout> layouts{};

		Sparse_Set<Draw_Data> draws{};

		bool is_material_equal(uint32_t a, uint32_t b) {
			return true;
		}

		void init() {
			Vertex_Input_Builder vertex_builder{};
			vertex_builder.add_binding_description(0, sizeof(Geometry::Vertex_2D), VK_VERTEX_INPUT_RATE_VERTEX)
				.add_attribute_description(0, VK_FORMAT_R32G32_SFLOAT, offsetof(Geometry::Vertex_2D, position));
			vertex_builder.add_binding_description(1, sizeof(Instance_Data), VK_VERTEX_INPUT_RATE_INSTANCE)
				.add_attribute_description(1, VK_FORMAT_R32_UINT, offsetof(Instance_Data, storage_offset))
				.add_attribute_description(1, VK_FORMAT_R32_UINT, offsetof(Instance_Data, config_offset));

			layouts.push_back(get_layout(Const::DESCRITPOR_LAYOUT::STORAGE_BUFFER));

			pipeline_config = make_default_pipeline_config();
			pipeline_config.vertex_binding_descriptions = vertex_builder.build_binding_descriptions();
			pipeline_config.attribute_descriptions = vertex_builder.build_attribute_descriptions();
			pipeline_config.descriptor_set_layouts = layouts;
			pipeline_config.vertex_shader_path = Const::PATH_VERT_SHADERD_DRAW_RECTANGLE_2D;
			pipeline_config.fragment_shader_path = Const::PATH_FRAG_SHADERD_DRAW_RECTANGLE_2D;

			pipeline.init(pipeline_config);

			Static_Buffer& static_buffer = get_static_buffer();
			vertex_id = static_buffer.upload_data(
				RECTANGLE_VERTICES.data(), sizeof(Geometry::Vertex_2D) * RECTANGLE_VERTICES.size(),
				sizeof(Geometry::Vertex_2D)
			);
			indices_id = static_buffer.upload_data(
				RECTANGLE_INDICES.data(), sizeof(uint16_t) * RECTANGLE_INDICES.size(), sizeof(uint16_t)
			);
		}

		size_t get_instance_size() {
			return sizeof(Instance_Data);
		}

		std::vector<Draw_Instance_Information> make_rectangle_2D(
			Draw_Information& draw_info, const Rectangle_Attributes& rectangle_attributes
		) {
			draw_info.draw_material_id = 0;
			SSBO_Buffer& ssbo = get_ssbo();
			Storage_Data storage{};
			storage.make(rectangle_attributes);
			uint32_t storage_id = ssbo.upload_data(&storage, sizeof(Storage_Data), sizeof(Storage_Data));
			const Buffer_Range& storage_range = ssbo.view_slot(storage_id);
			uint32_t config_draw_offset = Utils::get_draw_2D_config_data_offset(device);
			Instance_Data instance_data{storage_range.offset_as<Storage_Data>(), config_draw_offset};
			uint32_t instance_id = ssbo.upload_data(&instance_data, sizeof(Instance_Data));
			draws.insert(instance_id, {storage_id});
			draw_info.sub_unique_id = instance_id;
			return {{instance_id}};
		}

		void update_draw(uint32_t instance_id, const Rectangle_Attributes& rectangle_attributes) {
			SSBO_Buffer& ssbo = get_ssbo();
			Storage_Data storage{};
			storage.make(rectangle_attributes);
			const Draw_Data& draw = draws.get(instance_id);
			ssbo.update_data(draw.storage_id, &storage);
		}

		void draw(
			VkCommandBuffer command_buffer, uint32_t material_draw_id, uint32_t number_instance,
			uint32_t first_instance_offset, uint32_t frame_index
		) {
			Static_Buffer& static_buffer = get_static_buffer();
			Buffer_Range indices_range = static_buffer.view_slot(indices_id);
			Buffer_Range vertex_range = static_buffer.view_slot(vertex_id);
			Buffer instance_buffer = get_instance_buffer(frame_index);
			VkBuffer binding_buffers[2] = {static_buffer.inner_buffer.buffer, instance_buffer.buffer};
			VkDeviceSize buffer_offsets[2] = {0, 0};
			VkDescriptorSet storage_descriptor_set = get_storage_descriptor_set(frame_index);
			bind_draw_resource(
				command_buffer, pipeline.pipeline, pipeline.layout, static_buffer.inner_buffer.buffer, 0,
				VK_INDEX_TYPE_UINT16, 2, binding_buffers, buffer_offsets, 1, &storage_descriptor_set
			);
			vkCmdDrawIndexed(
				command_buffer, indices_range.size_as<uint16_t>(), number_instance, indices_range.offset_as<uint16_t>(),
				vertex_range.offset_as<Geometry::Vertex_2D>(), first_instance_offset / (uint32_t)(get_instance_size())
			);
		}

		void destroy() {
			pipeline.destroy();
		}

	} // namespace Rectangle

} // namespace Vulkan