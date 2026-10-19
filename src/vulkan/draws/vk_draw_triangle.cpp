#include <geometry_structs.h>
#include <math_custom.h>
#include <vulkan/draws/vk_draw_triangle.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_descriptor.h>
#include <vulkan/vk_draw.h>
#include <vulkan/vk_pipeline.h>
#include <vulkan/vk_utils.h>
#include <vulkan/vk_vertex_input_builder.h>

namespace Vulkan {

	namespace Triangle {

		struct Instance_Data {
			uint32_t storage_offset = 0;
			uint32_t vertex_offset = 0;
			uint32_t config_offset = 0;
		};

		struct Storage_Data {
			glm::vec2 first{0.f, 0.f};		// 8/8
			glm::vec2 second{0.f, 0.f};		// 8/8
			glm::vec3 color{1.f, 1.f, 1.f}; // 12/16
			float _pad = 0.f;				// 4
			glm::vec2 third{0.f, 0.f};		// 8/8
			float _pad2[2];					// 8/8
			void make(const Triangle_Attribultes& attributes) {
				first = attributes.first;
				second = attributes.second;
				third = attributes.third;
				color = attributes.color;
			}
		};

		struct Draw_Data {
			uint32_t storage_id = 0;
		};

		std::vector<Geometry::Vertex_2D> TRIANGLE_VERTICES = {{{0.f, 0.f}}, {{0.f, 0.f}}, {{0.f, .0f}}};

		std::vector<uint16_t> TRIANGLE_INDICES = {0, 1, 2};

		Pipeline_Config pipeline_config{};

		Pipeline pipeline{};

		std::vector<VkDescriptorSetLayout> layouts{};

		uint32_t vertex_id = 0;

		uint32_t indices_id = 0;

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
				.add_attribute_description(1, VK_FORMAT_R32_UINT, offsetof(Instance_Data, vertex_offset))
				.add_attribute_description(1, VK_FORMAT_R32_UINT, offsetof(Instance_Data, config_offset));

			Descriptor_Set_Layout_Builder layout_builder{};
			layouts.push_back(get_layout(Const::DESCRITPOR_LAYOUT::STORAGE_BUFFER));

			pipeline_config = Vulkan::make_default_pipeline_config();
			pipeline_config.vertex_binding_descriptions = vertex_builder.build_binding_descriptions();
			pipeline_config.attribute_descriptions = vertex_builder.build_attribute_descriptions();
			pipeline_config.descriptor_set_layouts = layouts;
			pipeline_config.vertex_shader_path = Const::PATH_VERT_SHADERD_DRAW_TRIANGLE_2D;
			pipeline_config.fragment_shader_path = Const::PATH_FRAG_SHADERD_DRAW_TRIANGLE_2D;

			pipeline.init(pipeline_config);

			Static_Buffer& static_buffer = get_static_buffer();
			vertex_id = static_buffer.upload_data(
				TRIANGLE_VERTICES.data(), sizeof(Geometry::Vertex_2D) * TRIANGLE_VERTICES.size(),
				sizeof(Geometry::Vertex_2D)
			);
			indices_id = static_buffer.upload_data(
				TRIANGLE_INDICES.data(), sizeof(uint16_t) * TRIANGLE_INDICES.size(), sizeof(uint16_t)
			);
		}

		size_t get_instance_size() {
			return sizeof(Instance_Data);
		}

		std::vector<Draw_Instance_Information> make_triangle(
			Draw_Information& draw_info, const Triangle_Attribultes& attributes
		) {
			if (!Math::is_valid_triangle_with_clockwise(attributes.first, attributes.second, attributes.third)) {
				throw std::runtime_error("Fail to make triangle invalid clockwise points!");
			}
			SSBO_Buffer& ssbo = get_ssbo();
			Storage_Data storage{};
			storage.make(attributes);
			uint32_t storage_id = ssbo.upload_data(&storage, sizeof(Storage_Data), sizeof(Storage_Data));
			const Buffer_Range& storage_range = ssbo.view_slot(storage_id);
			Static_Buffer& static_buffer = get_static_buffer();
			const Buffer_Range& vertex_range = static_buffer.view_slot(vertex_id);
			uint32_t config_draw_offset = Utils::get_draw_2D_config_data_offset(device);
			Instance_Data instance{
				storage_range.offset_as<Storage_Data>(), vertex_range.offset_as<Geometry::Vertex_2D>(),
				config_draw_offset
			};
			uint32_t instance_id = ssbo.upload_data(&instance, sizeof(Instance_Data), 1);
			draws.insert(instance_id, {storage_id});
			return {{instance_id}};
		}

		void update_triangle(uint32_t instance_id, const Triangle_Attribultes& attributes) {
			if (!Math::is_valid_triangle_with_clockwise(attributes.first, attributes.second, attributes.third)) {
				throw std::runtime_error(
					"Fail to update triangle invalid clockwise points: " + std::to_string(instance_id) + "!"
				);
			}
			SSBO_Buffer& ssbo = get_ssbo();
			const Draw_Data& draw = draws.get(instance_id);
			Storage_Data storage{};
			storage.make(attributes);
			ssbo.update_data(draw.storage_id, &instance);
		}

		void draw(
			VkCommandBuffer command_buffer, uint32_t material_draw_id, uint32_t number_instance,
			uint32_t first_instance_offset, uint32_t frame_index
		) {
			Static_Buffer& static_buffer = get_static_buffer();
			Buffer_Range indices_range = static_buffer.view_slot(indices_id);
			Buffer_Range vertex_range = static_buffer.view_slot(vertex_id);
			Buffer& instance_buffer = get_instance_buffer(frame_index);
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

	} // namespace Triangle

} // namespace Vulkan