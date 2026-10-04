#include <geometry_structs.h>
#include <log.h>
#include <utils.h>
#include <vulkan/draws/vk_draw_rectangle.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_pipeline.h>
#include <vulkan/vk_utils.h>
#include <vulkan/vk_vertex_input_builder.h>

namespace Vulkan {

	namespace Draw_2D {

		namespace Rectangle {

			struct Instance_Data {
				glm::vec2 translation{0.f, 0.f};
				glm::vec2 size{0.f, 0.f};
				glm::vec2 anchor{0.f, 0.f};
				glm::vec3 color{1.f, 1.f, 1.f};
				float rotation = 0.f;
				void make(const Rectangle_Attributes& attributes) {
					translation = attributes.position;
					size = glm::vec2(attributes.width, attributes.height);
					anchor = attributes.anchor;
					color = attributes.color;
					rotation = attributes.rotation;
				}
				friend std::ostream& operator<<(std::ostream& os, const Instance_Data& instance) {
					os << "{Rectangle::Instance_Data: translation: " << instance.translation
					   << ",  scale: " << instance.size << ", anchor: " << instance.anchor
					   << ", color: " << instance.color << ", rotation: " << instance.rotation << "}";
					return os;
				}
			};

			struct Push_Constants {
				glm::vec2 screen_size{0.f, 0.f};
			};

			std::vector<Geometry::Vertex_2D> RECTANGLE_VERTICES = {
				{{0.f, 0.f}}, {{0.f, 1.f}}, {{1.f, 1.f}}, {{1.f, 0.f}}
			};

			std::vector<uint16_t> RECTANGLE_INDICES = {0, 1, 2, 2, 3, 0};

			uint32_t vertex_id = 0;

			uint32_t indices_id = 0;

			Pipeline_Config pipeline_config{};

			Pipeline pipeline{};

			std::vector<VkDescriptorSetLayout> layouts{};

			bool is_material_equal(uint32_t a, uint32_t b) {
				return true;
			}

			void init() {
				Vertex_Input_Builder vertex_builder{};
				vertex_builder.add_binding_description(0, sizeof(Geometry::Vertex_2D), VK_VERTEX_INPUT_RATE_VERTEX)
					.add_attribute_description(0, VK_FORMAT_R32G32_SFLOAT, offsetof(Geometry::Vertex_2D, position));
				vertex_builder.add_binding_description(1, sizeof(Instance_Data), VK_VERTEX_INPUT_RATE_INSTANCE)
					.add_attribute_description(1, VK_FORMAT_R32G32_SFLOAT, offsetof(Instance_Data, translation))
					.add_attribute_description(1, VK_FORMAT_R32G32_SFLOAT, offsetof(Instance_Data, size))
					.add_attribute_description(1, VK_FORMAT_R32G32_SFLOAT, offsetof(Instance_Data, anchor))
					.add_attribute_description(1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Instance_Data, color))
					.add_attribute_description(1, VK_FORMAT_R32_SFLOAT, offsetof(Instance_Data, rotation));

				Descriptor_Set_Layout_Builder layout_builder{};
				layouts.push_back(layout_builder.build());

				pipeline_config = make_default_pipeline_config();
				pipeline_config.vertex_descriptions = vertex_builder.build_binding_descriptions();
				pipeline_config.attribute_descriptions = vertex_builder.build_attribute_descriptions();
				pipeline_config.descriptor_set_layouts = layouts;
				pipeline_config.push_constants_size = sizeof(Push_Constants);
				pipeline_config.vertex_shader_path = Const::PATH_VERT_SHADERD_DRAW_RECTANGLE_2D;
				pipeline_config.fragment_shader_path = Const::PATH_FRAG_SHADERD_DRAW_RECTANGLE_2D;
				pipeline_config.depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;

				pipeline.init(pipeline_config);

				Static_Buffer& vertex_buffer = get_vertex_buffer(sizeof(Geometry::Vertex_2D));
				Static_Buffer& indices_buffer = get_indices_buffer(sizeof(uint16_t));
				vertex_id = vertex_buffer.upload_data(
					sizeof(Geometry::Vertex_2D) * RECTANGLE_VERTICES.size(), RECTANGLE_VERTICES.data()
				);
				indices_id =
					indices_buffer.upload_data(sizeof(uint16_t) * RECTANGLE_INDICES.size(), RECTANGLE_INDICES.data());
				Vulkan::global_staging_buffer->flush_frame();
			}

			size_t get_instance_size() {
				return sizeof(Instance_Data);
			}

			Draw_2D_Information make_draw(const Rectangle_Attributes& rectangle_attributes) {
				Draw_2D_Information draw_info{Const::DRAW_RECTANGLE_2D};
				draw_info.draw_material_id = 0;
				SSBO_Buffer& ssbo = get_ssbo();
				Instance_Data instance_data{};
				instance_data.make(rectangle_attributes);
				draw_info.instance_id = ssbo.upload_data(&instance_data, sizeof(Instance_Data));
				return draw_info;
			}

			void update_draw(uint32_t instance_id, const Rectangle_Attributes& rectangle_attributes) {
				SSBO_Buffer& ssbo = get_ssbo();
				Instance_Data instance_data{};
				instance_data.make(rectangle_attributes);
				ssbo.update_data(instance_id, &instance_data);
			}

			void draw(
				VkCommandBuffer command_buffer, uint32_t material_draw_id, uint32_t number_instance,
				uint32_t first_instance_offset
			) {
				Static_Buffer& indices_buffer = get_indices_buffer(sizeof(uint16_t));
				Static_Buffer& vertex_buffer = get_vertex_buffer(sizeof(Geometry::Vertex_2D));
				Static_Buffer_Range indices_range = indices_buffer.view_slot_info(indices_id);
				Static_Buffer_Range vertex_range = vertex_buffer.view_slot_info(vertex_id);
				Buffer instance_buffer = get_instance_buffer();
				glm::vec2 screen_size = Utils::get_window_size();
				vkCmdPushConstants(
					command_buffer, pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
					offsetof(Push_Constants, screen_size), sizeof(glm::vec2), &screen_size
				);
				VkBuffer binding_buffers[2] = {vertex_buffer.inner_buffer.buffer, instance_buffer.buffer};
				VkDeviceSize buffer_offsets[2] = {0, 0};
				bind_draw_resource(
					command_buffer, pipeline.pipeline, pipeline.layout, indices_buffer.inner_buffer.buffer, 0,
					VK_INDEX_TYPE_UINT16, 2, binding_buffers, buffer_offsets, 0, nullptr
				);
				vkCmdDrawIndexed(
					command_buffer, indices_range.size_as<uint16_t>(), number_instance,
					indices_range.offset_as<uint16_t>(), vertex_range.offset_as<Geometry::Vertex_2D>(),
					first_instance_offset / (uint32_t)(get_instance_size())
				);
			}

			void destroy() {
				pipeline.destroy();
				for (VkDescriptorSetLayout layout : layouts) {
					vkDestroyDescriptorSetLayout(Vulkan::device, layout, nullptr);
				}
			}

		} // namespace Rectangle

	} // namespace Draw_2D

} // namespace Vulkan