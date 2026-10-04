#include <algorithm>
#include <map>
#include <profiler.h>
#include <sparse_set.h>
#include <tuple>
#include <vulkan/draws/vk_draw_rectangle.h>
#include <vulkan/draws/vk_draw_texture_2D.h>
#include <vulkan/draws/vk_draw_triangle.h>
#include <vulkan/vk_buffer.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_draw_2D.h>

namespace Vulkan {

	namespace Draw_2D {

		long long frame_count = 0;

		struct Group_Draw_Batching {
			Const::DRAW_ID draw_type = Const::DRAW_ID::UNDEFINED;
			uint32_t material_draw_id = 0;
			uint32_t instance_offset = 0;
			uint32_t number_instance = 0;
		};

		struct Binding_Draw_Info {
			VkPipeline pipeline = VK_NULL_HANDLE;
			VkBuffer vertex_buffer = VK_NULL_HANDLE;
			VkBuffer indices_buffer = VK_NULL_HANDLE;
			uint32_t vertex_binding_offset = 0;
			uint32_t indices_binding_offset = 0;
			VkIndexType index_type = VK_INDEX_TYPE_UINT16;
			std::vector<VkDeviceSize> vertex_buffer_offsets{};
			std::vector<VkBuffer> binding_vertex_buffers{};
			std::vector<VkDescriptorSet> binding_descriptor_sets{};
			void clean() {
				pipeline = VK_NULL_HANDLE;
				vertex_buffer = VK_NULL_HANDLE;
				indices_buffer = VK_NULL_HANDLE;
				vertex_binding_offset = 0;
				indices_binding_offset = 0;
				index_type = VK_INDEX_TYPE_UINT16;
				vertex_buffer_offsets.clear();
				binding_vertex_buffers.clear();
				binding_descriptor_sets.clear();
			}
		};

		struct Draw_Order_Information {
			uint32_t draw_index = 0;
			uint32_t create_index = 0;
			bool operator<(const Draw_Order_Information& other) const {
				return std::tie(draw_index, create_index) < std::tie(other.draw_index, other.create_index);
			}
			bool operator==(const Draw_Order_Information& other) const {
				return std::tie(draw_index, create_index) == std::tie(other.draw_index, other.create_index);
			}
		};

		Sparse_Set<Draw_2D_Information> draws;

		std::vector<uint32_t> sorted_draws;

		std::vector<Group_Draw_Batching> draw_groups;

		Buffer instance_buffer{};

		SSBO_Buffer ssbo_buffer{};

		std::map<size_t, Static_Buffer> vertex_buffers{};

		std::map<size_t, Static_Buffer> indices_buffers{};

		uint32_t current_create_index = 0;

		Binding_Draw_Info current_binding_draw_info{};

		std::map<Draw_Order_Information, uint32_t> draw_order_to_ids;

		const std::string SCOPE_SORT_DRAW = "2D SORT_DRAW";

		const std::string SCOPE_SET_UP_BUFFER = "2D SET_UP_BUFFER";

		const std::string SCOPE_BATCHING_GROUP = "2D BATCHING_GROUP";

		const std::string SCOPE_DRAW = "2D DRAW";

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
			Profiler::start_scope(SCOPE_SORT_DRAW);
			sorted_draws.clear();
			auto draw_ids = draws.keys();
			for (auto draw_id : draw_ids) {
				const auto& draw_info = draws.get(draw_id);
				if (draw_info.visible) {
					sorted_draws.push_back(draw_id);
				}
			}
			std::sort(sorted_draws.begin(), sorted_draws.end(), [](uint32_t a, uint32_t b) {
				const Draw_2D_Information& draw_a = draws.get(a);
				const Draw_2D_Information& draw_b = draws.get(b);
				if (draw_a.draw_index == draw_b.draw_index) {
					return draw_a.create_index < draw_b.create_index;
				}
				return draw_a.draw_index < draw_b.draw_index;
			});
			Profiler::end_scope(SCOPE_SORT_DRAW);
		}

		size_t get_instance_size(Const::DRAW_ID draw_type) {
			switch (draw_type) {
				case Const::DRAW_ID::DRAW_RECTANGLE_2D: {
					return Rectangle::get_instance_size();
				}
				case Const::DRAW_ID::DRAW_TEXTURE_2D: {
					return Texture_2D::get_instance_size();
				}
				case Const::DRAW_ID::DRAW_TRIANGLE_2D: {
					return Triangle::get_instance_size();
				}
				default: {
					throw std::runtime_error("Fail to get instance size, unsupport draw type!");
				}
			}
		}

		void setup_instance_buffer() {
			Profiler::start_scope(SCOPE_SET_UP_BUFFER);
			size_t size_reqiure = 0;
			for (int i = 0; i < sorted_draws.size(); i++) {
				uint32_t draw_id = sorted_draws[i];
				const Draw_2D_Information& draw_info = draws.get(draw_id);
				SSBO_Buffer_Range range = ssbo_buffer.view_slot(draw_info.instance_id);
				size_t instance_size = get_instance_size(draw_info.draw_type);
				if (size_reqiure % instance_size == 0) {
					size_reqiure += range.size;
				} else {
					size_t remain_size = instance_size - (size_reqiure % instance_size);
					size_reqiure += instance_size - remain_size + range.size;
				}
			}
			if (instance_buffer.size < size_reqiure) {
				uint32_t size = (uint32_t)(size_reqiure * 1.5f);
				instance_buffer.resize(size);
			}
			size_t offset = 0;
			for (int i = 0; i < sorted_draws.size(); i++) {
				uint32_t draw_id = sorted_draws[i];
				const Draw_2D_Information& draw_info = draws.get(draw_id);
				SSBO_Buffer_Range range = ssbo_buffer.view_slot(draw_info.instance_id);
				ssbo_buffer.transfer_data_to(draw_info.instance_id, instance_buffer.buffer, (uint32_t)offset);
				size_t instance_size = get_instance_size(draw_info.draw_type);
				if (offset % instance_size == 0) {
					offset += range.size;
				} else {
					size_t remain_size = instance_size - (size_reqiure % instance_size);
					offset += remain_size + range.size;
				}
			}
			ssbo_buffer.flush_transfer_data();
			Profiler::end_scope(SCOPE_SET_UP_BUFFER);
		}

		void batching_draw_groups() {
			Profiler::start_scope(SCOPE_BATCHING_GROUP);
			draw_groups.clear();
			if (sorted_draws.size() <= 0) {
				return;
			}
			uint32_t first_id = sorted_draws[0];
			Draw_2D_Information last_draw_info = draws.get(first_id);
			Group_Draw_Batching group{last_draw_info.draw_type, last_draw_info.draw_material_id, 0, 1};
			SSBO_Buffer_Range first_range = ssbo_buffer.view_slot(last_draw_info.instance_id);
			draw_groups.push_back(group);
			int current_group = 0;
			uint32_t current_instance_offset = first_range.size;
			for (int i = 1; i < sorted_draws.size(); i++) {
				uint32_t draw_id = sorted_draws[i];
				const Draw_2D_Information& draw_info = draws.get(draw_id);
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
			Profiler::end_scope(SCOPE_BATCHING_GROUP);
		}

		SSBO_Buffer& get_ssbo() {
			return ssbo_buffer;
		}

		Buffer& get_instance_buffer() {
			return instance_buffer;
		}

		void bind_draw_resource(
			VkCommandBuffer command_buffer, VkPipeline pipeline, VkPipelineLayout pipline_layout,
			VkBuffer indices_buffer, uint32_t indices_offset, VkIndexType index_type, uint32_t number_vertex_buffer,
			VkBuffer* binding_vertex_buffers, VkDeviceSize* vertex_buffer_offsets, uint32_t number_descriptor,
			VkDescriptorSet* binding_descriptor_sets
		) {
			if (current_binding_draw_info.pipeline != pipeline) {
				current_binding_draw_info.pipeline = pipeline;
				vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			}
			if (current_binding_draw_info.indices_buffer != indices_buffer ||
				current_binding_draw_info.indices_binding_offset != indices_offset ||
				current_binding_draw_info.index_type != index_type) {
				current_binding_draw_info.indices_buffer = indices_buffer;
				current_binding_draw_info.indices_binding_offset = indices_offset;
				current_binding_draw_info.index_type = index_type;
				vkCmdBindIndexBuffer(command_buffer, indices_buffer, indices_offset, index_type);
			}
			std::vector<VkBuffer>& last_buffers = current_binding_draw_info.binding_vertex_buffers;
			std::vector<VkDeviceSize>& last_offsets = current_binding_draw_info.vertex_buffer_offsets;
			if (number_vertex_buffer > last_buffers.size()) {
				last_buffers.resize(number_vertex_buffer);
				last_offsets.resize(number_vertex_buffer);
			}
			for (int i = 0; i < number_vertex_buffer; i++) {
				if (last_buffers[i] != binding_vertex_buffers[i] || last_offsets[i] != vertex_buffer_offsets[i]) {
					vkCmdBindVertexBuffers(command_buffer, i, 1, &binding_vertex_buffers[i], &vertex_buffer_offsets[i]);
					last_buffers[i] = binding_vertex_buffers[i];
					last_offsets[i] = vertex_buffer_offsets[i];
				}
			}
			std::vector<VkDescriptorSet>& last_descriptor_sets = current_binding_draw_info.binding_descriptor_sets;
			if (number_descriptor > last_descriptor_sets.size()) {
				last_descriptor_sets.resize(number_descriptor);
			}
			for (int i = 0; i < number_descriptor; i++) {
				if (last_descriptor_sets[i] != binding_descriptor_sets[i]) {
					vkCmdBindDescriptorSets(
						command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipline_layout, i, 1,
						&binding_descriptor_sets[i], 0, VK_NULL_HANDLE
					);
					last_descriptor_sets[i] = binding_descriptor_sets[i];
				}
			}
		}

		Static_Buffer& get_vertex_buffer(size_t vertex_size) {
			if (vertex_buffers.find(vertex_size) == vertex_buffers.end()) {
				Static_Buffer vertex_buffer{};
				vertex_buffer.init(
					Vulkan::global_staging_buffer.get(), Const::INITIALIZE_STATIC_BUFFER_SIZE,
					VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
				);
				vertex_buffers[vertex_size] = vertex_buffer;
			}
			return vertex_buffers[vertex_size];
		}

		Static_Buffer& get_indices_buffer(size_t indices_size) {
			if (indices_buffers.find(indices_size) == indices_buffers.end()) {
				Static_Buffer indices_buffer{};
				indices_buffer.init(
					Vulkan::global_staging_buffer.get(), Const::INITIALIZE_STATIC_BUFFER_SIZE,
					VK_BUFFER_USAGE_INDEX_BUFFER_BIT
				);
				indices_buffers[indices_size] = indices_buffer;
			}
			return indices_buffers[indices_size];
		}

		void init() {
			ssbo_buffer.init(Const::INITIALIZE_SIZE_STAGING_BUFFER);
			instance_buffer.make_buffer(
				Const::INITIALIZE_SIZE_INSTANCING_BUFFER, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			);
			/**
			 * Initialize all draw type.
			 */
			Texture_2D::init();
			Rectangle::init();
			Triangle::init();
		}

		void draw(VkCommandBuffer command_buffer) {
			sort_draws();
			setup_instance_buffer();
			batching_draw_groups();
			Profiler::start_scope(SCOPE_DRAW);
			current_binding_draw_info.clean();
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
			Profiler::end_scope(SCOPE_DRAW);
			frame_count++;
			if (frame_count % 60 == 0) {
				Profiler::view_scope(SCOPE_SORT_DRAW);
				Profiler::view_scope(SCOPE_SET_UP_BUFFER);
				Profiler::view_scope(SCOPE_BATCHING_GROUP);
				Profiler::view_scope(SCOPE_DRAW);
			}
		}

		void destroy() {
			Triangle::destroy();
			Texture_2D::destroy();
			Rectangle::destroy();
			instance_buffer.destroy();
			ssbo_buffer.destroy();
			for (auto& [size, buffer] : vertex_buffers) {
				buffer.destroy();
			}
			for (auto& [size, buffer] : indices_buffers) {
				buffer.destroy();
			}
		}

		void update_draw(uint32_t id, Draw_2D_Attribute draw_attributes) {
			if (!draws.has(id)) {
				return;
			}
			auto draw_info = draws.get(id);
			draw_info.draw_index = draw_attributes.draw_index;
			draw_info.visible = draw_attributes.is_visible;
		}

		uint32_t make_rectange(const Draw_2D_Attribute& draw_attributes, Rectangle_Attributes rectangle_attributes) {
			Draw_2D_Information draw_info = Rectangle::make_draw(rectangle_attributes);
			draw_info.draw_index = draw_attributes.draw_index;
			draw_info.visible = draw_attributes.is_visible;
			draw_info.create_index = ++current_create_index;
			return draws.insert(draw_info);
		}

		void update_rectangle(uint32_t id, Rectangle_Attributes rectangle_attributes) {
			if (!draws.has(id)) {
				return;
			}
			const Draw_2D_Information& draw_info = draws.get(id);
			Rectangle::update_draw(draw_info.instance_id, rectangle_attributes);
		}

		uint32_t make_texture_2D(
			const Draw_2D_Attribute& draw_attributes, const Texture_2D_Attributes& texture_attributes
		) {
			Draw_2D_Information draw_info = Texture_2D::make_texture_2D(texture_attributes);
			draw_info.draw_index = draw_attributes.draw_index;
			draw_info.visible = draw_attributes.is_visible;
			draw_info.create_index = ++current_create_index;
			return draws.insert(draw_info);
		}

		void update_texture_2D(uint32_t id, const Texture_2D_Attributes& texture_attributes) {
			if (!draws.has(id)) {
				return;
			}
			const Draw_2D_Information& draw_info = draws.get(id);
			Texture_2D::update_texture_2D(draw_info, texture_attributes);
		}

		uint32_t make_triangle(
			const Draw_2D_Attribute& draw_attributes, const Triangle_Attribultes& triangle_attributes
		) {
			Draw_2D_Information draw_info = Triangle::make_triangle(triangle_attributes);
			draw_info.draw_index = draw_attributes.draw_index;
			draw_info.visible = draw_attributes.is_visible;
			draw_info.create_index = ++current_create_index;
			return draws.insert(draw_info);
		}

		void update_triangle(uint32_t id, const Triangle_Attribultes& triangle_attributes) {
			if (!draws.has(id)) {
				return;
			}
			const Draw_2D_Information& draw_info = draws.get(id);
			Triangle::update_triangle(draw_info.instance_id, triangle_attributes);
		}

	} // namespace Draw_2D

} // namespace Vulkan