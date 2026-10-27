
#include <consts.h>
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

	namespace Draw_Font_2D {

		struct Instance_Data {
			uint32_t storage_offset = 0;
			uint32_t storage_char_offset = 0;
			uint32_t vertex_offset = 0;
			uint32_t config_draw_offset = 0;
			friend std::ostream& operator<<(std::ostream& os, const Instance_Data& instance) {
				os << "{Font_2D::Instance_Data: storage_offset: " << instance.storage_offset
				   << ", storage_char_offset: " << instance.storage_char_offset
				   << ", vertex_offset: " << instance.vertex_offset
				   << ", config_draw_offset: " << instance.config_draw_offset << "}";
				return os;
			}
		};

		struct Storage_Char_Data {
			glm::vec2 char_translation{0.f, 0.f}; // 8/8
			uint32_t char_offset = 0;			  // 4/4
			float _pad = 0.f;					  // 4/4
			friend std::ostream& operator<<(std::ostream& os, const Storage_Char_Data& instance) {
				os << "{Font_2D::Storage_Char_Data: char_translation: " << instance.char_translation
				   << ", char_offset: " << instance.char_offset << "}";
				return os;
			}
		};

		struct Storage_Data {
			glm::vec2 tex_size{0.f, 0.f};		   // 8/8
			glm::vec2 translation{0.f, 0.f};	   // 8/8
			glm::vec3 color{1.f, 1.f, 1.f};		   // 12/16
			float rotation = 0.f;				   // 4/4
			glm::vec2 scale{1.f, 1.f};			   // 8/8
			int32_t bucket_index = 0;			   // 4/4
			int32_t slot_index = 0;				   // 4/4
			glm::vec3 stroke_color{1.f, 1.f, 1.f}; // 12/16
			float stroke_size = 0.f;			   // 4/4
			friend std::ostream& operator<<(std::ostream& os, const Storage_Data& instance) {
				os << "{Rectangle::Storage_Data_2: translation: " << instance.translation
				   << ",  tex_size: " << instance.tex_size << ",  scale: " << instance.scale
				   << ", color: " << instance.color << ", rotation: " << instance.rotation << "}";
				return os;
			}
		};

		struct Text_Data {
			std::string text = "";
			int align = 0;
			std::vector<std::string> lines;
			std::vector<glm::vec2> start_line_positions;
			float width = 0;
			float height = 0;
			uint32_t storage_char_id = ::Const::MAX_UINT32;
			uint32_t storage_id = ::Const::MAX_UINT32;
			uint32_t font_id = ::Const::MAX_UINT32;
			glm::vec2 text_anchor{0.f};

			std::vector<Storage_Char_Data> make_char_storages_data(const Font& font, const Label_2D& attributes) {
				std::vector<Storage_Char_Data> char_storages;
				glm::vec2 origin = -glm::vec2(width, height) * attributes.get_anchor();
				for (int i = 0; i < lines.size(); i++) {
					const std::string& line = lines[i];
					const glm::vec2& start_line_position = start_line_positions[i];
					std::vector<glm::vec2> positions = get_char_position_at(
						line, start_line_position + origin, font, attributes.get_font_size(),
						attributes.get_stroke_size()
					);
					for (int j = 0; j < positions.size(); j++) {
						glm::vec2 position = positions[j];
						Storage_Char_Data char_storage{};
						char_storage.char_translation = position;
						char_storage.char_offset = font.get_char_rect_offset(line[j]);
						char_storages.push_back(char_storage);
					}
				}
				return char_storages;
			};

			Storage_Data make_storage_data(
				const Font& font, const Texture_View& texture_view, const Label_2D& attributes
			) {
				Storage_Data storage{};
				float char_scale = font.get_char_scale(attributes.get_font_size());
				storage.translation = attributes.get_position();
				storage.scale = attributes.get_scale() * char_scale;
				storage.tex_size = {font.size_atlas, font.size_atlas};
				storage.slot_index = texture_view.slot_index;
				storage.bucket_index = texture_view.bucket_index;
				storage.rotation = attributes.get_rotation();
				storage.color = attributes.get_color();
				storage.stroke_color = attributes.get_stroke_color();
				storage.stroke_size = font.claim_stroke(attributes.get_font_size(), attributes.get_stroke_size());
				return storage;
			}

			void clear(SSBO_Buffer& ssbo, std::vector<Draw_Instance_Information>& draw_instances) {
				clean(ssbo);
				storage_char_id = ::Const::MAX_UINT32;
				storage_id = ::Const::MAX_UINT32;
				text = "";
				text_anchor = {0.f, 0.f};
				draw_instances.clear();
			}

			void update_text_data(
				const Label_2D& attributes, SSBO_Buffer& ssbo, Static_Buffer& static_buffer,
				std::vector<Draw_Instance_Information>& draw_instances, uint32_t vertex_id
			) {
				uint32_t i_font_id = load_font(attributes.get_font_path());
				if (attributes.get_text() == "" || text != attributes.get_text() || font_id != i_font_id) {
					clear(ssbo, draw_instances);
					make(attributes, ssbo, static_buffer, draw_instances, vertex_id);
					return;
				}
				font_id = i_font_id;
				const Font& font = view_font(font_id);
				if (align != attributes.get_align() || text_anchor != attributes.get_anchor()) {
					make_lines_render_info(
						font, attributes.get_align(), attributes.get_font_size(), start_line_positions, width, height,
						lines, attributes.get_stroke_size()
					);
					std::vector<Storage_Char_Data> char_storages = make_char_storages_data(font, attributes);
					ssbo.update_data(storage_char_id, char_storages.data());
				}
				const Texture_View& texture_view = view_font_texture(font_id);
				Storage_Data storage = make_storage_data(font, texture_view, attributes);
				ssbo.update_data(storage_id, &storage);
				text = attributes.get_text();
				align = attributes.get_align();
				text_anchor = attributes.get_anchor();
			}

			void make(
				const Label_2D& attributes, SSBO_Buffer& ssbo, Static_Buffer& static_buffer,
				std::vector<Draw_Instance_Information>& draw_instances, uint32_t vertex_id
			) {
				if (attributes.get_text() == "") {
					text = "";
					draw_instances.clear();
					return;
				}
				text = attributes.get_text();
				align = attributes.get_align();
				text_anchor = attributes.get_anchor();
				font_id = load_font(attributes.get_font_path());
				const Font& font = view_font(font_id);
				break_lines(text, lines);
				make_lines_render_info(
					font, attributes.get_align(), attributes.get_font_size(), start_line_positions, width, height,
					lines, attributes.get_stroke_size()
				);
				const Texture_View& texture_view = view_font_texture(font_id);
				Storage_Data storage = make_storage_data(font, texture_view, attributes);
				storage_id = ssbo.upload_data(&storage, sizeof(Storage_Data), sizeof(Storage_Data));
				Buffer_Range storage_range = ssbo.view_slot(storage_id);
				Buffer_Range vertex_range = static_buffer.view_slot(vertex_id);
				std::vector<Storage_Char_Data> char_storages = make_char_storages_data(font, attributes);
				storage_char_id = ssbo.upload_data(
					char_storages.data(), char_storages.size() * sizeof(Storage_Char_Data), sizeof(Storage_Char_Data)
				);
				const Buffer_Range& storage_char_range = ssbo.view_slot(storage_char_id);
				std::vector<Instance_Data> instances{};
				uint32_t config_draw_offset = Utils::get_draw_2D_config_data_offset(device);
				for (uint32_t i = 0; i < char_storages.size(); i++) {
					instances.push_back(
						{storage_range.offset_as<Storage_Data>(), storage_char_range.offset_as<Storage_Char_Data>() + i,
						 vertex_range.offset_as<Geometry::Vertex_2D>(), config_draw_offset}
					);
				}
				uint32_t instance_id =
					ssbo.upload_data(instances.data(), instances.size() * sizeof(Instance_Data), sizeof(Instance_Data));
				draw_instances.push_back({instance_id, (uint32_t)char_storages.size()});
			}

			void clean(SSBO_Buffer& ssbo) const {
				if (storage_id == ::Const::MAX_UINT32)
					return;
				ssbo.remove_data(storage_id);
				ssbo.remove_data(storage_char_id);
			}
		};

		Id_Generator unique_id_generator{};

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

		void break_lines(std::string text, std::vector<std::string>& lines) {
			lines.clear();
			while (text.find("\n") != std::string::npos) {
				auto enter_char_index = text.find("\n");
				lines.push_back(text.substr(0, enter_char_index));
				text.erase(0, enter_char_index + 1);
			}
			if (text != "") {
				lines.push_back(text);
			}
		}

		glm::vec2 get_line_size(const std::string& line, const Font& font, int font_size, float stroke_size) {
			glm::vec2 line_size{0.f, font.line_height};
			for (int i = 0; i < line.size(); i++) {
				glm::vec2 char_bound_box = font.get_char_bounding_box(line[i], font_size, stroke_size);
				line_size.x += char_bound_box.x;
			}
			return line_size;
		}

		std::vector<glm::vec2> get_char_position_at(
			std::string line, glm::vec2 start_line_position, const Font& font, int font_size, float stroke_size
		) {
			std::vector<glm::vec2> char_positions(line.size());
			float start_x = 0;
			for (int i = 0; i < line.size(); i++) {
				char_positions[i] = start_line_position;
				glm::vec local_offset = font.get_char_local_offset(line[i], font_size, stroke_size);
				char_positions[i].x += start_x + local_offset.x;
				char_positions[i].y += local_offset.y;
				glm::vec2 char_bound_box = font.get_char_bounding_box(line[i], font_size, stroke_size);
				start_x += char_bound_box.x;
			}
			return char_positions;
		}

		void make_lines_render_info(
			const Font& font, int align, int font_size, std::vector<glm::vec2>& start_line_positions, float& width,
			float& height, const std::vector<std::string>& lines, float stroke_size
		) {
			start_line_positions.resize(lines.size());
			float max_width_line = 0;
			for (const std::string& line : lines) {
				auto size = get_line_size(line, font, font_size, stroke_size);
				max_width_line = std::max(size.x, max_width_line);
			}
			float total_height = 0;
			for (int i = lines.size() - 1; i > -1; i--) {
				const std::string& line = lines[i];
				glm::vec2 line_size = get_line_size(line, font, font_size, stroke_size);
				float line_x = 0;
				switch (align) {
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
			float char_scale = font.get_char_scale(font_size);
			width = max_width_line * char_scale;
			height = total_height * char_scale;
		}

		Text_Helper gen_text_info_from(
			std::string font_path, std::string text, int align, int font_size, float stroke_size
		) {
			Text_Helper text_helper{};
			uint32_t font_id = load_font(font_path);
			const Font& font = view_font(font_id);
			break_lines(text, text_helper.lines);
			make_lines_render_info(
				font, align, font_size, text_helper.start_line_positions, text_helper.width, text_helper.height,
				text_helper.lines, stroke_size
			);
			return text_helper;
		}

		void draw(
			VkCommandBuffer command_buffer, uint32_t material_draw_id, uint32_t number_instance,
			uint32_t first_instance_offset, int frame_index
		) {
			Static_Buffer& static_buffer = get_static_buffer();
			Buffer_Range indices_range = static_buffer.view_slot(indices_id);
			Buffer_Range vertex_range = static_buffer.view_slot(vertex_id);
			Buffer instance_buffer = get_instance_buffer(frame_index);
			VkDescriptorSet storage_descriptor_set = get_storage_descriptor_set(frame_index);
			std::array<VkDescriptorSet, 2> font_texture_descriptor_sets =
				get_descriptor_set_font_sampler(material_draw_id, frame_index);
			uint32_t number_descriptor_set = 0;
			VkDescriptorSet using_descriptor_sets[3] = {
				storage_descriptor_set, font_texture_descriptor_sets[0], font_texture_descriptor_sets[1]
			};
			if (Const::ENABLED_TEXTURE_BUCKETS) {
				number_descriptor_set = 3;
			} else {
				number_descriptor_set = 2;
			}
			VkBuffer binding_buffers[2] = {static_buffer.inner_buffer.buffer, instance_buffer.buffer};
			VkDeviceSize buffer_offsets[2] = {0, 0};
			bind_draw_resource(
				command_buffer, pipeline.pipeline, pipeline.layout, static_buffer.inner_buffer.buffer, 0,
				VK_INDEX_TYPE_UINT16, 2, binding_buffers, buffer_offsets, number_descriptor_set, using_descriptor_sets
			);
			vkCmdDrawIndexed(
				command_buffer, indices_range.size_as<uint16_t>(), number_instance, indices_range.offset_as<uint16_t>(),
				vertex_range.offset_as<Geometry::Vertex_2D>(), first_instance_offset / (uint32_t)(get_instance_size())
			);
		}

		void init_font(
			Draw_Information& draw_info, std::vector<Draw_Instance_Information>& draw_instances,
			const Label_2D& attributes
		) {
			uint32_t unique_id = unique_id_generator.gen_id();
			Text_Data text{};
			text.make(attributes, get_ssbo(), get_static_buffer(), draw_instances, vertex_id);
			draw_info.draw_material_id = text.font_id;
			draw_info.sub_unique_id = unique_id;
			texts.insert(unique_id, text);
		}

		void update_font(
			Draw_Information& draw_info, std::vector<Draw_Instance_Information>& draw_instances,
			const Label_2D& attributes
		) {
			if (!texts.has(draw_info.sub_unique_id)) {
				return;
			}
			Text_Data& text = texts.modify(draw_info.sub_unique_id);
			text.update_text_data(attributes, get_ssbo(), get_static_buffer(), draw_instances, vertex_id);
			draw_info.draw_material_id = text.font_id;
		}

		void remove_font(uint32_t sub_unique_id) {
			if (!texts.has(sub_unique_id)) {
				return;
			}
			const Text_Data& text = texts.get(sub_unique_id);
			text.clean(get_ssbo());
			texts.erase(sub_unique_id);
			unique_id_generator.release_id(sub_unique_id);
		}

		void destroy() {
			pipeline.destroy();
		}

	} // namespace Draw_Font_2D

} // namespace Vulkan