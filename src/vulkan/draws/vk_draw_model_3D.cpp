#include <geometry_structs.h>
#include <id_generator.h>
#include <sparse_set.h>
#include <vulkan/draws/vk_draw_model_3D.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_descriptor.h>
#include <vulkan/vk_pipeline.h>
#include <vulkan/vk_vertex_input_builder.h>

namespace Vulkan {

	namespace Model_3D {

		struct Instance_Data {
			glm::mat4 transform{1.f};
		};

		struct Push_Constants {
			glm::mat4 mesh_transform{1.f};
		};

		struct Material_Data {
			uint32_t model_id = 0;
		};

		Pipeline_Config pipeline_config{};

		Pipeline pipeline{};

		Sparse_Set<Material_Data> material_by_ids{};

		std::vector<VkDescriptorSetLayout> layouts{};

		std::vector<VkDescriptorSet> texture_bucket_descriptor_sets{};

		std::vector<VkDescriptorSet> default_texture_descriptor_sets{};

		void init() {
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
			if (Const::ENABLED_TEXTURE_BUCKETS) {
				layouts.push_back(texture_system.make_bucket_descriptor_set_layout());
				default_texture_descriptor_sets = texture_system.make_default_texture_descriptor_sets(layouts[0]);
				texture_bucket_descriptor_sets = texture_system.make_bucket_descriptor_sets(layouts[1]);
			} else {
				layout_builder.clear();
				layouts.push_back(layout_builder.build(device));
			}
			layout_builder.clear();
			layout_builder.add_binding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
			layouts.push_back(layout_builder.build(device));

			pipeline_config = make_default_pipeline_config();
			pipeline_config.attribute_descriptions = vertex_builder.build_attribute_descriptions();
			pipeline_config.vertex_binding_descriptions = vertex_builder.build_binding_descriptions();
			pipeline_config.descriptor_set_layouts = layouts;
			pipeline_config.vertex_shader_path = Const::PATH_VERT_SHADERD_DRAW_MODEL_3D;
			pipeline_config.fragment_shader_path = Const::PATH_FRAG_SHADERD_DRAW_MODEL_3D;
			pipeline_config.depth_compare_op = VK_COMPARE_OP_LESS;
			pipeline_config.triangle_trip_order = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			pipeline_config.push_constants_size = sizeof(Push_Constants);

			pipeline.init(pipeline_config);
		}

		void destroy() {
			for (const VkDescriptorSetLayout& layout : layouts) {
				vkDestroyDescriptorSetLayout(device, layout, nullptr);
			}
			pipeline.destroy();
		}

	} // namespace Model_3D

} // namespace Vulkan