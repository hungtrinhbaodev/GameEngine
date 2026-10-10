
#include <geometry_structs.h>
#include <log.h>
#include <sparse_set.h>
#include <vulkan/draws/vk_draw_font_2D.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_descriptor.h>
#include <vulkan/vk_font_system.h>
#include <vulkan/vk_pipeline.h>
#include <vulkan/vk_structs.h>
#include <vulkan/vk_utils.h>
#include <vulkan/vk_vertex_input_builder.h>

namespace Vulkan {

	namespace Font_2D {

		struct Instance_Data {
			glm::vec2 tex_size{0.f, 0.f};
			glm::vec2 translation{0.f, 0.f};
			glm::vec2 char_translation{0.f, 0.f};
			glm::vec2 scale{1.f, 1.f};
			glm::vec4 rect{0.f, 0.f, 0.f, 0.f};
			glm::vec3 color{1.f, 1.f, 1.f};
			float rotation = 0.f;
			uint32_t bucket_index = 0;
			uint32_t slot_index = 0;
			friend std::ostream& operator<<(std::ostream& os, const Instance_Data& instance) {
				os << "{Rectangle::Instance_Data: translation: " << instance.translation
				   << ",  tex_size: " << instance.tex_size << ",  char_translation: " << instance.char_translation
				   << ",  scale: " << instance.scale << ", color: " << instance.color << ", rect: " << instance.rect
				   << ", rotation: " << instance.rotation << "}";
				return os;
			}
		};

		struct Push_Constants {
			glm::vec2 screen_size{0.f, 0.f};
			uint32_t vertex_offset = 0;
		};

		struct Text_Data {
			std::vector<Instance_Data> chars{};
			Font_2D_Attributes attributes{};
			float width = 0;
			float height = 0;

			std::vector<std::string> break_lines(std::string& text) {
				std::vector<std::string> lines{};
				while (text.find("\n") != std::string::npos) {
					auto enter_char_index = text.find("\n");
					lines.push_back(text.substr(0, enter_char_index));
					text.erase(0, enter_char_index + 1);
				}
				if (text != "") {
					lines.push_back(text);
				}
				return lines;
			}

			glm::vec2 get_line_size(const std::string& line, const Font& font) {
				glm::vec2 line_size{0.f, font.line_height};
				for (int i = 0; i < line.size(); i++) {
					glm::vec2 char_bound_box = font.get_char_bounding_box(line[i]);
					line_size.x += char_bound_box.x;
				}
				return line_size;
			}

			std::vector<glm::vec2> get_char_position_at(
				std::string line, glm::vec2 start_line_position, const Font& font
			) {
				std::vector<glm::vec2> char_positions(line.size());
				float start_x = 0;
				for (int i = 0; i < line.size(); i++) {
					char_positions[i] = start_line_position;
					glm::vec local_offset = font.get_char_local_offset(line[i]);
					char_positions[i].x += start_x + local_offset.x;
					char_positions[i].y += local_offset.y;
					glm::vec2 char_bound_box = font.get_char_bounding_box(line[i]);
					start_x += char_bound_box.x;
				}
				return char_positions;
			}

			bool make(const Font_2D_Attributes& attributes, const Font& font, Texture_System& texture_system) {
				if (this->attributes == attributes) {
					return false;
				}
				if (attributes.text == "") {
					return false;
				}
				std::string text = attributes.text;
				std::vector<std::string> lines = break_lines(text);
				std::vector<glm::vec2> start_line_positions(lines.size());
				float max_width_line = 0;
				for (const std::string& line : lines) {
					auto size = get_line_size(line, font);
					max_width_line = std::max(size.x, max_width_line);
				}
				float total_height = 0;
				for (int i = lines.size() - 1; i > -1; i--) {
					const std::string& line = lines[i];
					glm::vec2 line_size = get_line_size(line, font);
					float line_x = 0;
					switch (attributes.align) {
						case 0: {
							line_x = (max_width_line - line_size.x) / 2;
							break;
						}
						case 1: {
							line_x = 0;
							break;
						}
						default: {
							line_x = max_width_line - line_size.x;
							break;
						}
					}
					start_line_positions[i] = {line_x, total_height};
					total_height += line_size.y;
				}
				width = max_width_line;
				height = total_height;
				glm::vec2 origin = -glm::vec2(width, height) * attributes.anchor;
				chars.clear();
				Texture_View texture_view = texture_system.view_texture(font.texture_id);
				float char_scale = font.get_char_scale(attributes.font_size);
				for (int i = 0; i < lines.size(); i++) {
					const std::string& line = lines[i];
					const glm::vec2 start_line_position = start_line_positions[i];
					std::vector<glm::vec2> positions = get_char_position_at(line, start_line_position + origin, font);
					for (int j = 0; j < positions.size(); j++) {
						glm::vec2 position = positions[j];
						Instance_Data instance{};
						instance.translation = attributes.position;
						instance.char_translation = position;
						instance.scale = attributes.scale * char_scale;
						instance.tex_size = {font.size_atlas, font.size_atlas};
						instance.slot_index = texture_view.slot_index;
						instance.bucket_index = texture_view.bucket_index;
						instance.rotation = attributes.rotation;
						instance.color = attributes.color;
						auto char_rect = font.get_char_rect(line[j]);
						instance.rect = char_rect.to_vec4();
						chars.push_back(instance);
					}
				}
				this->attributes = attributes;
				width *= char_scale;
				height *= char_scale;
				return true;
			}
		};

		std::vector<Geometry::Vertex_2D> FONT_VERTICES = {{{0.f, 0.f}}, {{0.f, 1.f}}, {{1.f, 1.f}}, {{1.f, 0.f}}};

		std::vector<uint16_t> FONT_INDICES = {0, 1, 2, 2, 3, 0};

		Sparse_Set<Text_Data> texts;

		Pipeline_Config pipeline_config{};

		Pipeline pipeline{};

		std::unordered_map<uint32_t, std::vector<VkDescriptorSet>> texture_descriptor_sets{};

		std::vector<VkDescriptorSet> texture_default_descriptor_sets{};

		std::vector<VkDescriptorSet> texture_bucket_descriptor_sets{};

		std::vector<VkDescriptorSetLayout> layouts{};

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
				.add_attribute_description(1, VK_FORMAT_R32G32_SFLOAT, offsetof(Instance_Data, char_translation))
				.add_attribute_description(1, VK_FORMAT_R32G32_SFLOAT, offsetof(Instance_Data, scale))
				.add_attribute_description(1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Instance_Data, rect))
				.add_attribute_description(1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Instance_Data, color))
				.add_attribute_description(1, VK_FORMAT_R32_SFLOAT, offsetof(Instance_Data, rotation))
				.add_attribute_description(1, VK_FORMAT_R32_SINT, offsetof(Instance_Data, bucket_index))
				.add_attribute_description(1, VK_FORMAT_R32_SINT, offsetof(Instance_Data, slot_index));

			Descriptor_Set_Layout_Builder layout_builder{};
			layout_builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
			layouts.push_back(layout_builder.build(Vulkan::device));
			if (Const::ENABLED_TEXTURE_BUCKETS) {
				layouts.push_back(font_system.texture_system.make_bucket_descriptor_set_layout());
				texture_default_descriptor_sets =
					Vulkan::texture_system.make_default_texture_descriptor_sets(layouts[0]);
				texture_bucket_descriptor_sets = font_system.texture_system.make_bucket_descriptor_sets(layouts[1]);
			}

			pipeline_config = Vulkan::make_default_pipeline_config();
			pipeline_config.vertex_binding_descriptions = vertex_builder.build_binding_descriptions();
			pipeline_config.attribute_descriptions = vertex_builder.build_attribute_descriptions();
			pipeline_config.descriptor_set_layouts = layouts;
			pipeline_config.push_constants_size = sizeof(Push_Constants);
			pipeline_config.vertex_shader_path = Const::PATH_VERT_SHADERD_DRAW_FONT_2D;
			pipeline_config.fragment_shader_path = Const::PATH_FRAG_SHADERD_DRAW_FONT_2D;
			if (Const::ENABLED_TEXTURE_BUCKETS) {
				pipeline_config.fragment_shader_path = Const::PATH_FRAG_SHADERD_DRAW_FONT_2D_USING_BUCKET;
			}
			pipeline.init(pipeline_config);

			Static_Buffer_2& static_buffer = get_static_buffer();
			vertex_id = static_buffer.upload_data(
				FONT_VERTICES.data(), sizeof(Geometry::Vertex_2D) * FONT_VERTICES.size(), sizeof(Geometry::Vertex_2D)
			);
			indices_id = static_buffer.upload_data(
				FONT_INDICES.data(), sizeof(uint16_t) * FONT_INDICES.size(), sizeof(uint16_t)
			);
		}

		size_t get_instance_size() {
			return sizeof(Instance_Data);
		}

		void draw(
			VkCommandBuffer command_buffer, uint32_t material_draw_id, uint32_t number_instance,
			uint32_t first_instance_offset, int frame_index
		) {
			Static_Buffer_2& static_buffer = get_static_buffer();
			Static_Buffer_Range_2 indices_range = static_buffer.view_slot_info(indices_id);
			Static_Buffer_Range_2 vertex_range = static_buffer.view_slot_info(vertex_id);
			Buffer instance_buffer = get_instance_buffer();
			const Font& font = font_system.view_font(material_draw_id);
			Texture_View texture_view = font_system.texture_system.view_texture(font.texture_id);
			uint32_t number_descriptor_set = 0;
			VkDescriptorSet* binding_descriptor_sets = nullptr;
			if (!Const::ENABLED_TEXTURE_BUCKETS) {
				VkDescriptorSet using_descriptor_sets[1] = {texture_descriptor_sets[font.texture_id][frame_index]};
				number_descriptor_set = 1;
				binding_descriptor_sets = using_descriptor_sets;
			} else {
				VkDescriptorSet using_descriptor_sets[2] = {};
				if (texture_view.storage_mode == Const::TEXTURE_STORAGE_MODE::BUCKET) {
					using_descriptor_sets[0] = texture_default_descriptor_sets[frame_index];
				} else {
					using_descriptor_sets[0] = texture_descriptor_sets[font.texture_id][frame_index];
				}
				using_descriptor_sets[1] = texture_bucket_descriptor_sets[frame_index];
				number_descriptor_set = 2;
				binding_descriptor_sets = using_descriptor_sets;
			}
			VkBuffer binding_buffers[2] = {static_buffer.inner_buffer.buffer, instance_buffer.buffer};
			VkDeviceSize buffer_offsets[2] = {0, 0};
			bind_draw_resource(
				command_buffer, pipeline.pipeline, pipeline.layout, static_buffer.inner_buffer.buffer, 0,
				VK_INDEX_TYPE_UINT16, 2, binding_buffers, buffer_offsets, number_descriptor_set, binding_descriptor_sets
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

		Draw_Information make_font_2D(const Font_2D_Attributes& font_attributes) {
			uint32_t font_id = font_system.load_font(font_attributes.path);
			const Font& font = font_system.view_font(font_id);
			Texture_View texture_view = font_system.texture_system.view_texture(font.texture_id);
			if (texture_descriptor_sets.find(font.texture_id) == texture_descriptor_sets.end() &&
				texture_view.storage_mode == Const::TEXTURE_STORAGE_MODE::INDIVIDUAL) {
				texture_descriptor_sets[font.texture_id] = Utils::make_texture_descriptor_sets(
					descriptor_pools, layouts[1], texture_view.image.get_descriptor_info(texture_view.slot_index),
					device
				);
			}
			Draw_Information draw_info{Const::DRAW_FONT_2D};
			Text_Data text{};
			text.make(font_attributes, font, font_system.texture_system);
			SSBO_Buffer& ssbo = get_ssbo();
			draw_info.instance_id = ssbo.upload_data(text.chars.data(), text.chars.size() * sizeof(Instance_Data));
			draw_info.draw_material_id = font_id;
			texts.insert(draw_info.instance_id, text);
			return draw_info;
		}

		void destroy() {
			pipeline.destroy();
			for (const auto& layout : layouts) {
				vkDestroyDescriptorSetLayout(Vulkan::device, layout, nullptr);
			}
		}

	} // namespace Font_2D

} // namespace Vulkan