#include <algorithm>
#include <array>
#include <id_generator.h>
#include <map>
#include <profiler.h>
#include <sparse_set.h>
#include <tuple>
#include <vulkan/draws/vk_draw_font_2D.h>
#include <vulkan/draws/vk_draw_model_3D.h>
#include <vulkan/draws/vk_draw_rectangle.h>
#include <vulkan/draws/vk_draw_texture_2D.h>
#include <vulkan/draws/vk_draw_triangle.h>
#include <vulkan/vk_buffer.h>
#include <vulkan/vk_common.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_draw.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

	long long frame_count = 0;

	const std::string SCOPE_SORT_DRAW = "2D SORT_DRAW";

	const std::string SCOPE_SET_UP_BUFFER = "2D SET_UP_BUFFER";

	const std::string SCOPE_BATCHING_GROUP = "2D BATCHING_GROUP";

	const std::string SCOPE_DRAW = "2D DRAW";

	struct Group_Draw_Batching {
		Const::DRAW_ID draw_type = Const::DRAW_ID::UNDEFINED;
		uint32_t material_draw_id = 0;
		uint32_t instance_offset = 0;
		uint32_t number_instance = 0;
		uint32_t instance_size = 0;
	};

	struct Draw_Sort_Information {
		uint32_t draw_id = 0;
		uint32_t draw_index = 0;
		uint32_t create_index = 0;
	};

	struct Binding_Draw_Information {
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

	Id_Generator draw_id_generator{};

	Sparse_Set<Draw_Information> draws{};

	std::vector<Draw_Sort_Information> sorted_draws{};

	std::vector<Group_Draw_Batching> draw_groups{};

	uint32_t current_create_index = 0;

	Binding_Draw_Information current_binding_draw_info{};

	VkDescriptorSetLayout texture_layout{};

	VkDescriptorSetLayout texture_bucket_layout{};

	std::array<Buffer, Const::MAX_FRAMES_IN_FLIGHT> instance_buffers{};

	std::vector<VkBufferCopy> instances_copy_data{};

	std::array<Resize_Buffer_Synchronize_Information, Const::MAX_FRAMES_IN_FLIGHT> frame_resize_instance_buffer{};

	bool is_same_draw(const Draw_Information& a, const Draw_Information& b) {
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
			case Const::DRAW_ID::DRAW_FONT_2D: {
				return Font_2D::is_material_equal(a.draw_material_id, b.draw_material_id);
			}
			case Const::DRAW_ID::DRAW_MODEL_3D: {
				return Model_3D::is_material_equal(a.draw_material_id, b.draw_material_id);
			}
			default: {
				throw std::runtime_error("Fail to compare material, unsupport draw type!");
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
				sorted_draws.push_back({draw_id, draw_info.draw_index, draw_info.create_index});
			}
		}
		std::sort(
			sorted_draws.begin(), sorted_draws.end(),
			[](const Draw_Sort_Information& a, const Draw_Sort_Information& b) {
				if (a.draw_index == b.draw_index) {
					return a.create_index < b.create_index;
				}
				return a.draw_index < b.draw_index;
			}
		);
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
			case Const::DRAW_ID::DRAW_FONT_2D: {
				return Font_2D::get_instance_size();
			}
			case Const::DRAW_ID::DRAW_MODEL_3D: {
				return Model_3D::get_instance_size();
			}
			default: {
				throw std::runtime_error("Fail to get instance size, unsupport draw type!");
			}
		}
	}

	void record_setup_draw_buffers(VkCommandBuffer command_buffer, uint32_t current_frame) {
		Profiler::start_scope(SCOPE_SET_UP_BUFFER);
		Resize_Buffer_Synchronize_Information resize_info = frame_resize_instance_buffer[current_frame];
		if (resize_info.is_resizing) {
			resize_info.old_buffer.destroy();
			resize_info.is_resizing = false;
		}
		size_t size_reqiure = 0;
		SSBO_Buffer& ssbo = get_ssbo();
		Buffer& instance_buffer = get_instance_buffer(current_frame);
		for (int i = 0; i < sorted_draws.size(); i++) {
			uint32_t draw_id = sorted_draws[i].draw_id;
			const Draw_Information& draw_info = draws.get(draw_id);
			Buffer_Range range = ssbo.view_slot(draw_info.instance_id);
			size_t instance_size = get_instance_size(draw_info.draw_type);
			size_t remain_size = Utils::calculate_range_remain_size(size_reqiure, instance_size);
			size_reqiure += remain_size + range.inner_size();
		}
		if (instance_buffer.size < size_reqiure) {
			uint32_t size = (uint32_t)(size_reqiure * 1.5f);
			resize_info.is_resizing = true;
			resize_info.old_buffer = instance_buffer.record_resize(command_buffer, size);
		}
		size_t offset = 0;
		instances_copy_data.clear();
		for (int i = 0; i < sorted_draws.size(); i++) {
			uint32_t draw_id = sorted_draws[i].draw_id;
			const Draw_Information& draw_info = draws.get(draw_id);
			Buffer_Range range = ssbo.view_slot(draw_info.instance_id);
			size_t instance_size = get_instance_size(draw_info.draw_type);
			Buffer_Range ssbo_range = ssbo.view_slot(draw_info.instance_id);
			size_t remain_size = Utils::calculate_range_remain_size(offset, instance_size);
			instances_copy_data.push_back(
				{(VkDeviceSize)ssbo_range.inner_offset(), offset + remain_size, (VkDeviceSize)ssbo_range.inner_size()}
			);
			offset += remain_size + range.inner_size();
		}
		Utils::record_buffer_barrier(
			command_buffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, instance_buffer.buffer, 0, instance_buffer.size
		);
		{
			vkCmdCopyBuffer(
				command_buffer, ssbo.inner_buffer.buffer, instance_buffer.buffer, instances_copy_data.size(),
				instances_copy_data.data()
			);
		}
		Utils::record_buffer_barrier(
			command_buffer, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, instance_buffer.buffer, 0, instance_buffer.size
		);
		Model_3D::record_upload_uniform_buffer(command_buffer, current_frame);
		Profiler::end_scope(SCOPE_SET_UP_BUFFER);
	}

	void batching_draw_groups() {
		Profiler::start_scope(SCOPE_BATCHING_GROUP);
		draw_groups.clear();
		if (sorted_draws.size() <= 0) {
			return;
		}
		uint32_t first_id = sorted_draws[0].draw_id;
		Draw_Information last_draw_info = draws.get(first_id);
		size_t instance_size = get_instance_size(last_draw_info.draw_type);
		SSBO_Buffer& ssbo = get_ssbo();
		Buffer_Range first_range = ssbo.view_slot(last_draw_info.instance_id);
		Group_Draw_Batching group{
			last_draw_info.draw_type, last_draw_info.draw_material_id, 0,
			(uint32_t)(first_range.inner_size() / instance_size), (uint32_t)instance_size
		};
		draw_groups.push_back(group);
		int current_group = 0;
		uint32_t current_instance_offset = first_range.inner_size();
		for (int i = 1; i < sorted_draws.size(); i++) {
			uint32_t draw_id = sorted_draws[i].draw_id;
			const Draw_Information& draw_info = draws.get(draw_id);
			size_t instance_size = get_instance_size(draw_info.draw_type);
			Buffer_Range range = ssbo.view_slot(draw_info.instance_id);
			uint32_t remain = Utils::calculate_range_remain_size(current_instance_offset, instance_size);
			if (is_same_draw(last_draw_info, draw_info)) {
				draw_groups[current_group].number_instance += (uint32_t)(range.inner_size() / instance_size);
			} else {
				Group_Draw_Batching group{
					draw_info.draw_type, draw_info.draw_material_id, current_instance_offset + remain,
					(uint32_t)(range.inner_size() / instance_size), (uint32_t)instance_size
				};
				last_draw_info = draw_info;
				draw_groups.push_back(group);
				current_group++;
			}
			current_instance_offset += remain + range.inner_size();
		}
		Profiler::end_scope(SCOPE_BATCHING_GROUP);
	}

	Buffer& get_instance_buffer(uint32_t frame_index) {
		return instance_buffers[frame_index];
	}

	void bind_draw_resource(
		VkCommandBuffer command_buffer, VkPipeline pipeline, VkPipelineLayout pipline_layout, VkBuffer indices_buffer,
		uint32_t indices_offset, VkIndexType index_type, uint32_t number_vertex_buffer,
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
			if (last_descriptor_sets[i] != binding_descriptor_sets[i] && binding_descriptor_sets[i] != VK_NULL_HANDLE) {
				vkCmdBindDescriptorSets(
					command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipline_layout, i, 1, &binding_descriptor_sets[i],
					0, VK_NULL_HANDLE
				);
				last_descriptor_sets[i] = binding_descriptor_sets[i];
			}
		}
	}

	void init_draw() {
		{
			for (int i = 0; i < instance_buffers.size(); i++) {
				Buffer& instance = instance_buffers[i];
				instance.make_buffer(
					Const::INITIALIZE_SIZE_INSTANCING_BUFFER, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, Vulkan::physical_device, Vulkan::device
				);
			}
		}
		{
			/**
			 * Initialize all draw type.
			 */
			Texture_2D::init();
			Rectangle::init();
			Triangle::init();
			Font_2D::init();
			Model_3D::init();
		}
	}

	void setup_draw() {
		Model_3D::setup_draw(Vulkan::current_frame);
	}

	void draw(VkCommandBuffer command_buffer) {
		batching_draw_groups();
		Profiler::start_scope(SCOPE_DRAW);
		current_binding_draw_info.clean();
		std::vector<Group_Draw_Batching>& draw_groups2 = draw_groups;
		for (const Group_Draw_Batching& group : draw_groups) {
			switch (group.draw_type) {
				case Const::DRAW_ID::DRAW_RECTANGLE_2D: {
					Rectangle::draw(
						command_buffer, group.material_draw_id, group.number_instance, group.instance_offset,
						Vulkan::current_frame
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
						command_buffer, group.material_draw_id, group.number_instance, group.instance_offset,
						Vulkan::current_frame
					);
					break;
				}
				case Const::DRAW_ID::DRAW_FONT_2D: {
					Font_2D::draw(
						command_buffer, group.material_draw_id, group.number_instance, group.instance_offset,
						Vulkan::current_frame
					);
					break;
				}
				case Const::DRAW_ID::DRAW_MODEL_3D: {
					Model_3D::draw(
						command_buffer, group.material_draw_id, group.number_instance, group.instance_offset,
						Vulkan::current_frame
					);
					break;
				}
				default: {
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

	void destroy_draw() {
		Model_3D::destroy();
		Font_2D::destroy();
		Triangle::destroy();
		Texture_2D::destroy();
		Rectangle::destroy();
		for (const Buffer& buffer : instance_buffers) {
			buffer.destroy();
		}
	}

	void update_draw(uint32_t id, Draw_Attribute draw_attributes) {
		if (!draws.has(id)) {
			return;
		}
		auto& draw_info = draws.modify(id);
		draw_info.draw_index = draw_attributes.draw_index;
		draw_info.visible = draw_attributes.is_visible;
	}

	uint32_t make_rectange(const Draw_Attribute& draw_attributes, Rectangle_Attributes rectangle_attributes) {
		uint32_t draw_id = draw_id_generator.gen_id();
		Draw_Information draw_info = Rectangle::make_draw(rectangle_attributes);
		draw_info.draw_index = draw_attributes.draw_index;
		draw_info.visible = draw_attributes.is_visible;
		draw_info.create_index = ++current_create_index;
		return draws.insert(draw_id, draw_info);
	}

	void update_rectangle(uint32_t id, Rectangle_Attributes rectangle_attributes) {
		if (!draws.has(id)) {
			return;
		}
		const Draw_Information& draw_info = draws.get(id);
		Rectangle::update_draw(draw_info.instance_id, rectangle_attributes);
	}

	uint32_t make_texture_2D(const Draw_Attribute& draw_attributes, const Texture_2D_Attributes& texture_attributes) {
		uint32_t draw_id = draw_id_generator.gen_id();
		Draw_Information draw_info = Texture_2D::make_texture_2D(texture_attributes);
		draw_info.draw_index = draw_attributes.draw_index;
		draw_info.visible = draw_attributes.is_visible;
		draw_info.create_index = ++current_create_index;
		return draws.insert(draw_id, draw_info);
	}

	void update_texture_2D(uint32_t id, const Texture_2D_Attributes& texture_attributes) {
		if (!draws.has(id)) {
			return;
		}
		const Draw_Information& draw_info = draws.get(id);
		Texture_2D::update_texture_2D(draw_info, texture_attributes);
	}

	uint32_t make_triangle(const Draw_Attribute& draw_attributes, const Triangle_Attribultes& triangle_attributes) {
		uint32_t draw_id = draw_id_generator.gen_id();
		Draw_Information draw_info = Triangle::make_triangle(triangle_attributes);
		draw_info.draw_index = draw_attributes.draw_index;
		draw_info.visible = draw_attributes.is_visible;
		draw_info.create_index = ++current_create_index;
		return draws.insert(draw_id, draw_info);
	}

	uint32_t make_font_2D(const Draw_Attribute& draw_attributes, const Font_2D_Attributes& font_attributes) {
		uint32_t draw_id = draw_id_generator.gen_id();
		Draw_Information draw_info = Font_2D::make_font_2D(font_attributes);
		draw_info.draw_index = draw_attributes.draw_index;
		draw_info.visible = draw_attributes.is_visible;
		draw_info.create_index = ++current_create_index;
		return draws.insert(draw_id, draw_info);
	}

	uint32_t make_model_3D(const Draw_Attribute& draw_attributes, const Model_3D_Attributes& model_attributes) {
		uint32_t draw_id = draw_id_generator.gen_id();
		Draw_Information draw_info = Model_3D::make_model_3D(model_attributes);
		draw_info.draw_index = draw_attributes.draw_index;
		draw_info.visible = draw_attributes.is_visible;
		draw_info.create_index = ++current_create_index;
		return draws.insert(draw_id, draw_info);
	}

	void update_triangle(uint32_t id, const Triangle_Attribultes& triangle_attributes) {
		if (!draws.has(id)) {
			return;
		}
		const Draw_Information& draw_info = draws.get(id);
		Triangle::update_triangle(draw_info.instance_id, triangle_attributes);
	}

} // namespace Vulkan