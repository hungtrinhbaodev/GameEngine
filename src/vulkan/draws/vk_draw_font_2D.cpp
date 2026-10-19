
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
			uint32_t storage_offset = 0;
			uint32_t storage_char_offset = 0;
			uint32_t vertex_offset = 0;
			uint32_t config_draw_offset = 0;
		};

		struct Storage_Char_Data {
			glm::vec2 char_translation{0.f, 0.f}; // 8/8
			uint32_t char_offset = 0;			  // 4/4
			float _pad = 0.f;					  // 4/4
			friend std::ostream& operator<<(std::ostream& os, const Storage_Char_Data& instance) {
				os << "{Rectangle::Storage_Char_Data: char_translation: " << instance.char_translation
				   << ", char_offset: " << instance.char_offset << "}";
				return os;
			}
		};

		struct Storage_Data {
			glm::vec2 tex_size{0.f, 0.f};	 // 8/8
			glm::vec2 translation{0.f, 0.f}; // 8/8
			glm::vec3 color{1.f, 1.f, 1.f};	 // 12/16
			float rotation = 0.f;			 // 4/4
			glm::vec2 scale{1.f, 1.f};		 // 8/8
			int32_t bucket_index = 0;		 // 4/4
			int32_t slot_index = 0;			 // 4/4
			friend std::ostream& operator<<(std::ostream& os, const Storage_Data& instance) {
				os << "{Rectangle::Storage_Data_2: translation: " << instance.translation
				   << ",  tex_size: " << instance.tex_size << ",  scale: " << instance.scale
				   << ", color: " << instance.color << ", rotation: " << instance.rotation << "}";
				return os;
			}
		};

		struct Text_Data {
			std::vector<uint32_t> storage_ids{};
			uint32_t storage_id = 0;
			std::vector<uint32_t> storage_char_ids{};
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

			bool is_changing(const Font_2D_Attributes& attributes) const { return this->attributes != attributes; }

			void make_2(
				const Font_2D_Attributes& attributes, const Font& font, const Texture_View& texture_view,
				std::vector<Storage_Char_Data>& char_storages, Storage_Data& storage
			) {
				if (this->attributes == attributes) {
					return;
				}
				if (attributes.text == "") {
					return;
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
				float char_scale = font.get_char_scale(attributes.font_size);
				for (int i = 0; i < lines.size(); i++) {
					const std::string& line = lines[i];
					const glm::vec2 start_line_position = start_line_positions[i];
					std::vector<glm::vec2> positions = get_char_position_at(line, start_line_position + origin, font);
					for (int j = 0; j < positions.size(); j++) {
						glm::vec2 position = positions[j];
						Storage_Char_Data char_storage{};
						char_storage.char_translation = position;
						char_storage.char_offset = font.get_char_rect_offset(line[j]);
						char_storages.push_back(char_storage);
					}
				}
				storage.translation = attributes.position;
				storage.scale = attributes.scale * char_scale;
				storage.tex_size = {font.size_atlas, font.size_atlas};
				storage.slot_index = texture_view.slot_index;
				storage.bucket_index = texture_view.bucket_index;
				storage.rotation = attributes.rotation;
				storage.color = attributes.color;
				this->attributes = attributes;
				width *= char_scale;
				height *= char_scale;
			}
		};

		std::vector<Geometry::Vertex_2D> FONT_VERTICES = {{{0.f, 0.f}}, {{0.f, 1.f}}, {{1.f, 1.f}}, {{1.f, 0.f}}};

		std::vector<uint16_t> FONT_INDICES = {0, 1, 2, 2, 3, 0};

		Sparse_Set<Text_Data> texts;

		Pipeline_Config pipeline_config{};

		Pipeline pipeline{};

		std::vector<VkDescriptorSetLayout> layouts{};

		uint32_t vertex_id = 0;

		uint32_t indices_id = 0;

		bool is_material_equal(uint32_t a, uint32_t b) {
			if (a == b)
				return true;
			const Texture_View& texture_view_a = view_font_texture(a);
			const Texture_View& texture_view_b = view_font_texture(b);
			return texture_view_a.storage_mode == Const::TEXTURE_STORAGE_MODE::BUCKET &&
				   texture_view_b.storage_mode == Const::TEXTURE_STORAGE_MODE::BUCKET;
		}

		void init() {
			Vertex_Input_Builder vertex_builder{};
			vertex_builder.add_binding_description(0, sizeof(Geometry::Vertex_2D), VK_VERTEX_INPUT_RATE_VERTEX)
				.add_attribute_description(0, VK_FORMAT_R32G32_SFLOAT, offsetof(Geometry::Vertex_2D, position));
			vertex_builder.add_binding_description(1, sizeof(Instance_Data), VK_VERTEX_INPUT_RATE_INSTANCE)
				.add_attribute_description(1, VK_FORMAT_R32_UINT, offsetof(Instance_Data, storage_offset))
				.add_attribute_description(1, VK_FORMAT_R32_UINT, offsetof(Instance_Data, storage_char_offset))
				.add_attribute_description(1, VK_FORMAT_R32_UINT, offsetof(Instance_Data, vertex_offset))
				.add_attribute_description(1, VK_FORMAT_R32_UINT, offsetof(Instance_Data, config_draw_offset));

			layouts.push_back(get_layout(Const::DESCRITPOR_LAYOUT::STORAGE_BUFFER));
			layouts.push_back(get_layout(Const::DESCRITPOR_LAYOUT::IMAGE_SAMPLER));
			if (Const::ENABLED_TEXTURE_BUCKETS) {
				layouts.push_back(get_layout(Const::DESCRITPOR_LAYOUT::BUCKET_IMAGE_SAMPLER));
			}

			pipeline_config = Vulkan::make_default_pipeline_config();
			pipeline_config.vertex_binding_descriptions = vertex_builder.build_binding_descriptions();
			pipeline_config.attribute_descriptions = vertex_builder.build_attribute_descriptions();
			pipeline_config.descriptor_set_layouts = layouts;
			pipeline_config.vertex_shader_path = Const::PATH_VERT_SHADERD_DRAW_FONT_2D;
			pipeline_config.fragment_shader_path = Const::PATH_FRAG_SHADERD_DRAW_FONT_2D;
			if (Const::ENABLED_TEXTURE_BUCKETS) {
				pipeline_config.fragment_shader_path = Const::PATH_FRAG_SHADERD_DRAW_FONT_2D_USING_BUCKET;
			}
			pipeline.init(pipeline_config);

			Static_Buffer& static_buffer = get_static_buffer();
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
			Static_Buffer& static_buffer = get_static_buffer();
			Buffer_Range indices_range = static_buffer.view_slot(indices_id);
			Buffer_Range vertex_range = static_buffer.view_slot(vertex_id);
			Buffer instance_buffer = get_instance_buffer(frame_index);
			uint32_t number_descriptor_set = 0;
			VkDescriptorSet* binding_descriptor_sets = nullptr;
			VkDescriptorSet storage_descriptor_set = get_storage_descriptor_set(frame_index);
			std::array<VkDescriptorSet, 2> font_texture_descriptor_sets =
				get_descriptor_set_font_sampler(material_draw_id, frame_index);
			if (Const::ENABLED_TEXTURE_BUCKETS) {
				VkDescriptorSet using_descriptor_sets[3] = {
					storage_descriptor_set, font_texture_descriptor_sets[0], font_texture_descriptor_sets[1]
				};
				number_descriptor_set = 3;
				binding_descriptor_sets = using_descriptor_sets;
			} else {
				VkDescriptorSet using_descriptor_sets[2] = {storage_descriptor_set, font_texture_descriptor_sets[0]};
				number_descriptor_set = 2;
				binding_descriptor_sets = using_descriptor_sets;
			}
			VkBuffer binding_buffers[2] = {static_buffer.inner_buffer.buffer, instance_buffer.buffer};
			VkDeviceSize buffer_offsets[2] = {0, 0};
			bind_draw_resource(
				command_buffer, pipeline.pipeline, pipeline.layout, static_buffer.inner_buffer.buffer, 0,
				VK_INDEX_TYPE_UINT16, 2, binding_buffers, buffer_offsets, number_descriptor_set, binding_descriptor_sets
			);
			vkCmdDrawIndexed(
				command_buffer, indices_range.size_as<uint16_t>(), number_instance, indices_range.offset_as<uint16_t>(),
				vertex_range.offset_as<Geometry::Vertex_2D>(), first_instance_offset / (uint32_t)(get_instance_size())
			);
		}

		uint32_t update_text_data(
			Draw_Information& draw_info, Text_Data& text, const Font_2D_Attributes& text_attributes
		) {
			uint32_t font_id = load_font(text_attributes.path);
			const Font& font = view_font(font_id);
			Texture_View texture_view = view_font_texture(font_id);
			std::vector<Storage_Char_Data> storage_chars{};
			Storage_Data storage{};
			text.make_2(text_attributes, font, texture_view, storage_chars, storage);
			SSBO_Buffer& ssbo = get_ssbo();
			text.storage_ids.clear();
			std::vector<Instance_Data> instances{};
			uint32_t storage_id = ssbo.upload_data(&storage, sizeof(Storage_Data), sizeof(Storage_Data));
			Buffer_Range storage_range = ssbo.view_slot(storage_id);
			Static_Buffer& static_buffer = get_static_buffer();
			Buffer_Range vertex_range = static_buffer.view_slot(vertex_id);
			uint32_t config_draw_offset = Utils::get_draw_2D_config_data_offset(device);
			text.storage_id = storage_id;
			text.storage_char_ids.clear();
			for (Storage_Char_Data& storage : storage_chars) {
				uint32_t storage_char_id =
					ssbo.upload_data(&storage, sizeof(Storage_Char_Data), sizeof(Storage_Char_Data));
				const Buffer_Range& storage_char_range = ssbo.view_slot(storage_char_id);
				instances.push_back(
					{storage_range.offset_as<Storage_Data>(), storage_char_range.offset_as<Storage_Char_Data>(),
					 vertex_range.offset_as<Geometry::Vertex_2D>(), config_draw_offset}
				);
				text.storage_char_ids.push_back(storage_char_id);
			}
			uint32_t instance_id =
				ssbo.upload_data(instances.data(), instances.size() * sizeof(Instance_Data), sizeof(Instance_Data));
			draw_info.sub_unique_id = instance_id;
			draw_info.draw_material_id = font_id;
			return instance_id;
		}

		std::vector<Draw_Instance_Information> make_font_2D(
			Draw_Information& draw_info, const Font_2D_Attributes& text_attributes
		) {
			Text_Data text{};
			uint32_t instance_id = update_text_data(draw_info, text, text_attributes);
			texts.insert(instance_id, text);
			return {{instance_id, (uint32_t)text.storage_char_ids.size()}};
		}

		void update_font_2D(Draw_Information& draw_info, const Font_2D_Attributes& text_attributes) {
			const Text_Data& text = texts.get(draw_info.sub_unique_id);
			if (!text.is_changing(text_attributes)) {
				return;
			}
			SSBO_Buffer& ssbo = get_ssbo();
			for (const uint32_t& char_storage_id : text.storage_char_ids) {
				ssbo.remove_data(char_storage_id);
			}
			ssbo.remove_data(text.storage_id);
			ssbo.remove_data(draw_info.sub_unique_id);
			texts.erase(draw_info.sub_unique_id);
			Text_Data new_text{};
			uint32_t instance_id = update_text_data(draw_info, new_text, text_attributes);
			texts.insert(instance_id, new_text);
		}

		void destroy() {
			pipeline.destroy();
		}

	} // namespace Font_2D

} // namespace Vulkan