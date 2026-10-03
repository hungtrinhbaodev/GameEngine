#include <algorithm>
#include <id_generator.h>
#include <map>
#include <vulkan/draws/vk_draw_rectangle.h>
#include <vulkan/draws/vk_draw_texture_2D.h>
#include <vulkan/draws/vk_draw_triangle.h>
#include <vulkan/vk_buffer.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_draw_2D.h>

namespace Vulkan {

	namespace Draw_2D {

		struct Group_Draw_Batching {
			Const::DRAW_ID draw_type = Const::DRAW_ID::UNDEFINED;
			uint32_t material_draw_id = 0;
			uint32_t instance_offset = 0;
			uint32_t number_instance = 0;
		};

		Id_Generator draw_id_generator{};

		std::map<uint32_t, Draw_2D_Information> draws;

		std::vector<uint32_t> sorted_draws;

		std::vector<Group_Draw_Batching> draw_groups;

		Buffer instance_buffer{};

		SSBO_Buffer ssbo_buffer{};

		Static_Buffer vertex_buffer{};

		Static_Buffer indices_buffer{};

		uint32_t current_create_index = 0;

		bool is_same_draw(const Draw_2D_Information& a, const Draw_2D_Information& b) {
			if (a.draw_type != b.draw_type)
				return false;
			switch (a.draw_type) {
				case Const::DRAW_ID::DRAW_RECTANGLE_2D: {
					return Rectangle::is_material_equal(a.draw_material_id, b.draw_material_id);
				}
				case Const::DRAW_ID::DRAW_TEXTURE_2D: {
					return Texture_2D::is_material_equal(a.draw_material_id, b.draw_material_id);
				}
				case Const::DRAW_ID::DRAW_TRIANGLE_2D: {
					return Triangle::is_material_equal(a.draw_material_id, b.draw_material_id);
				}
				default: {
					return false;
				}
			}
		}

		void sort_draws() {
			sorted_draws.clear();
			for (const auto& [draw_id, draw_information] : draws) {
				if (!draw_information.visible) {
					continue;
				}
				sorted_draws.push_back(draw_id);
			}
			std::sort(sorted_draws.begin(), sorted_draws.end(), [](uint32_t a, uint32_t b) {
				if (draws[a].draw_index == draws[b].draw_index) {
					return draws[a].create_index < draws[b].create_index;
				}
				return draws[a].draw_index < draws[b].draw_index;
			});
		}

		void setup_instance_buffer() {
			uint32_t size_reqiure = 0;
			for (int i = 0; i < sorted_draws.size(); i++) {
				uint32_t draw_id = sorted_draws[i];
				Draw_2D_Information draw_info = draws[draw_id];
				SSBO_Buffer_Range range = ssbo_buffer.view_slot(draw_info.instance_id);
				size_reqiure += range.size;
			}
			if (instance_buffer.size < size_reqiure) {
				uint32_t size = (uint32_t)(size_reqiure * 1.5f);
				instance_buffer.resize(size);
			}
			uint32_t offset = 0;
			for (int i = 0; i < sorted_draws.size(); i++) {
				uint32_t draw_id = sorted_draws[i];
				Draw_2D_Information draw_info = draws[draw_id];
				SSBO_Buffer_Range range = ssbo_buffer.view_slot(draw_info.instance_id);
				ssbo_buffer.transfer_data_to(draw_info.instance_id, instance_buffer.buffer, offset);
				offset += range.size;
			}
			ssbo_buffer.flush_transfer_data();
		}

		void batching_draw_groups() {
			draw_groups.clear();
			if (sorted_draws.size() <= 0) {
				return;
			}
			uint32_t first_id = sorted_draws[0];
			Draw_2D_Information last_draw_info = draws[first_id];
			Group_Draw_Batching group{last_draw_info.draw_type, last_draw_info.draw_material_id, 0, 1};
			SSBO_Buffer_Range first_range = ssbo_buffer.view_slot(last_draw_info.instance_id);
			draw_groups.push_back(group);
			int current_group = 0;
			uint32_t current_instance_offset = first_range.size;
			for (int i = 1; i < sorted_draws.size(); i++) {
				uint32_t draw_id = sorted_draws[i];
				Draw_2D_Information draw_info = draws[draw_id];
				if (is_same_draw(last_draw_info, draw_info)) {
					draw_groups[current_group].number_instance++;
				} else {
					Group_Draw_Batching group{
						draw_info.draw_type, draw_info.draw_material_id, current_instance_offset, 1
					};
					last_draw_info = draw_info;
					draw_groups.push_back(group);
					current_group++;
				}
				SSBO_Buffer_Range range = ssbo_buffer.view_slot(draw_info.instance_id);
				current_instance_offset += range.size;
			}
		}

		SSBO_Buffer& get_ssbo() {
			return ssbo_buffer;
		}

		Static_Buffer& get_vertex_buffer() {
			return vertex_buffer;
		}

		Static_Buffer& get_indices_buffer() {
			return indices_buffer;
		}

		Buffer& get_instance_buffer() {
			return instance_buffer;
		}

		void init() {
			ssbo_buffer.init(Const::INITIALIZE_SIZE_STAGING_BUFFER);
			instance_buffer.make_buffer(
				Const::INITIALIZE_SIZE_INSTANCING_BUFFER, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);
			vertex_buffer.init(
				Vulkan::global_staging_buffer.get(), Const::INITIALIZE_STATIC_BUFFER_SIZE,
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
			);
			indices_buffer.init(
				Vulkan::global_staging_buffer.get(), Const::INITIALIZE_STATIC_BUFFER_SIZE,
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT
			);
			/**
			 * Initialize all draw type.
			 */
			Rectangle::init();
			Texture_2D::init();
			Triangle::init();
		}

		void draw(VkCommandBuffer command_buffer) {
			sort_draws();
			setup_instance_buffer();
			batching_draw_groups();
			std::vector<Group_Draw_Batching>& draw_groups2 = draw_groups;
			for (const Group_Draw_Batching& group : draw_groups) {
				switch (group.draw_type) {
					case Const::DRAW_ID::DRAW_RECTANGLE_2D: {
						Rectangle::draw(
							command_buffer, group.material_draw_id, group.number_instance, group.instance_offset
						);
						break;
					}
					case Const::DRAW_ID::DRAW_TEXTURE_2D: {
						Texture_2D::draw(
							command_buffer, group.material_draw_id, group.number_instance, group.instance_offset,
							Vulkan::current_frame
						);
						break;
					}
					case Const::DRAW_ID::DRAW_TRIANGLE_2D: {
						Triangle::draw(
							command_buffer, group.material_draw_id, group.number_instance, group.instance_offset
						);
						break;
					}
				}
			}
		}

		void destroy() {
			Triangle::destroy();
			Texture_2D::destroy();
			Rectangle::destroy();
			instance_buffer.destroy();
			ssbo_buffer.destroy();
			vertex_buffer.destroy();
			indices_buffer.destroy();
		}

		void update_draw(uint32_t id, Draw_2D_Attribute draw_attributes) {
			if (draws.find(id) == draws.end()) {
				return;
			}
			draws[id].draw_index = draw_attributes.draw_index;
			draws[id].visible = draw_attributes.is_visible;
		}

		uint32_t make_rectange(const Draw_2D_Attribute& draw_attributes, Rectangle_Attributes rectangle_attributes) {
			uint32_t id = draw_id_generator.gen_id();
			Draw_2D_Information draw_info = Rectangle::make_draw(rectangle_attributes);
			draw_info.draw_index = draw_attributes.draw_index;
			draw_info.visible = draw_attributes.is_visible;
			draw_info.create_index = ++current_create_index;
			draws[id] = draw_info;
			return id;
		}

		void update_rectangle(uint32_t id, Rectangle_Attributes rectangle_attributes) {
			if (draws.find(id) == draws.end()) {
				return;
			}
			const Draw_2D_Information& draw_info = draws[id];
			Rectangle::update_draw(draw_info.instance_id, rectangle_attributes);
		}

		uint32_t make_texture_2D(
			const Draw_2D_Attribute& draw_attributes, const Texture_2D_Attributes& texture_attributes
		) {
			uint32_t id = draw_id_generator.gen_id();
			Draw_2D_Information draw_info = Texture_2D::make_texture_2D(texture_attributes);
			draw_info.draw_index = draw_attributes.draw_index;
			draw_info.visible = draw_attributes.is_visible;
			draw_info.create_index = ++current_create_index;
			draws[id] = draw_info;
			return id;
		}

		void update_texture_2D(uint32_t id, const Texture_2D_Attributes& texture_attributes) {
			if (draws.find(id) == draws.end()) {
				return;
			}
			const Draw_2D_Information& draw_info = draws[id];
			Texture_2D::update_texture_2D(draw_info, texture_attributes);
		}

		uint32_t make_triangle(
			const Draw_2D_Attribute& draw_attributes, const Triangle_Attribultes& triangle_attributes
		) {
			uint32_t id = draw_id_generator.gen_id();
			Draw_2D_Information draw_info = Triangle::make_triangle(triangle_attributes);
			draw_info.draw_index = draw_attributes.draw_index;
			draw_info.visible = draw_attributes.is_visible;
			draw_info.create_index = ++current_create_index;
			draws[id] = draw_info;
			return id;
		}

		void update_triangle(uint32_t id, const Triangle_Attribultes& triangle_attributes) {
			if (draws.find(id) == draws.end()) {
				return;
			}
			const Draw_2D_Information& draw_info = draws[id];
			Triangle::update_triangle(draw_info.instance_id, triangle_attributes);
		}

	} // namespace Draw_2D

} // namespace Vulkan