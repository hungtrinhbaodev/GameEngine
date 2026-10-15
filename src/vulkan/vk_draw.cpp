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
#include <vulkan/vk_core.h>
#include <vulkan/vk_draw.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

	long long frame_count = 0;

	const std::string SCOPE_SORT_DRAW = "2D SORT_DRAW";

	const std::string SCOPE_BATCHING_GROUP = "2D BATCHING_GROUP";

	const std::string SCOPE_DRAW = "2D DRAW";

	struct Draw_Group_Batching {
		Const::DRAW_ID draw_type = Const::DRAW_ID::UNDEFINED;
		uint32_t start_index = 0;
		uint32_t number_element = 0;
		uint32_t draw_material_id = 0;
	};

	struct Draw_Group_Instance_Batching {
		Const::DRAW_ID draw_type = Const::DRAW_ID::UNDEFINED;
		uint32_t draw_material_id = 0;
		uint32_t instance_material_id = 0;
		uint32_t offset = 0;
		uint32_t number_instance = 0;
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

	Id_Generator draw_id_generator{};

	Sparse_Set<Draw_Information> draw_id_to_info{};

	Sparse_Set<std::vector<Draw_Instance_Information>> draw_id_to_instance{};

	std::vector<Draw_Sort_Information> sorted_draws{};

	std::vector<Draw_Group_Batching> draw_groups{};

	std::vector<Draw_Instance_Information> draw_instances{};

	std::vector<Draw_Instance_Information> draw_instance_in_groups{};

	std::vector<Draw_Group_Instance_Batching> draw_instance_batching_groups{};

	uint32_t current_create_index = 0;

	Binding_Draw_Information current_binding_draw_info{};

	VkDescriptorSetLayout texture_layout{};

	VkDescriptorSetLayout texture_bucket_layout{};

	std::array<Buffer, Const::MAX_FRAMES_IN_FLIGHT> instance_buffers{};

	std::vector<VkBufferCopy> instances_copy_data{};

	std::array<Resize_Buffer_Synchronize_Information, Const::MAX_FRAMES_IN_FLIGHT> frame_resize_instance_buffer{};

	std::unordered_map<Const::DESCRITPOR_LAYOUT, VkDescriptorSetLayout> layouts{};

	std::array<VkDescriptorSet, Const::MAX_FRAMES_IN_FLIGHT> storage_buffer_descriptor_sets{};

	Sparse_Set<std::array<VkDescriptorSet, Const::MAX_FRAMES_IN_FLIGHT>> frame_texture_descriptor_sets{};

	Sparse_Set<std::array<VkDescriptorSet, Const::MAX_FRAMES_IN_FLIGHT>> frame_font_descriptor_sets{};

	std::array<VkDescriptorSet, Const::MAX_FRAMES_IN_FLIGHT> texture_bucket_descriptor_sets{};

	std::array<VkDescriptorSet, Const::MAX_FRAMES_IN_FLIGHT> default_texture_descriptor_sets{};

	std::array<VkDescriptorSet, Const::MAX_FRAMES_IN_FLIGHT> default_font_descriptor_sets{};

	std::array<VkDescriptorSet, Const::MAX_FRAMES_IN_FLIGHT> font_bucket_descriptor_sets{};

	std::array<VkDescriptorSet, Const::MAX_FRAMES_IN_FLIGHT> uniform_buffer_descriptor_sets{};

	std::vector<uint32_t> group_copy_instance_ids{};

	void init_common_descriptor_set_layout() {
		Descriptor_Set_Layout_Builder layout_builder{};
		/**
		 * Init descritpor set layout for storage buffer.
		 */
		for (int i = 0; i < Const::MAX_DESCRIPTOR_STORAGE_BUFFER_BINDING; i++) {
			layout_builder.add_binding(i, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
		}
		VkDescriptorSetLayout storage_buffer_layout = layout_builder.build(device);
		SSBO_Buffer& ssbo = get_ssbo();
		for (int i = 0; i < Const::MAX_FRAMES_IN_FLIGHT; i++) {
			VkDescriptorSet descriptor_set =
				Structs::make_descriptor_set(descriptor_pools_2[i], &storage_buffer_layout, device);
			Buffer& storage_buffer = ssbo.storage_buffers[i];
			Descriptor_Set_Writer writer{};
			for (int j = 0; j < Const::MAX_DESCRIPTOR_STORAGE_BUFFER_BINDING; j++) {
				writer
					.add_buffer_write(j, &storage_buffer.descriptor, descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
					.write(device);
				writer.clear();
			}
			storage_buffer_descriptor_sets[i] = descriptor_set;
		}
		layouts[Const::DESCRITPOR_LAYOUT::STORAGE_BUFFER] = storage_buffer_layout;
		/**
		 * Init descriptor set layout for texture.
		 */
		layout_builder.clear();
		layout_builder.add_binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
		VkDescriptorSetLayout sampler_layout = layout_builder.build(device);
		default_texture_descriptor_sets = texture_system.make_default_texture_descriptor_sets_2(sampler_layout);
		default_font_descriptor_sets =
			font_system.texture_system.make_default_texture_descriptor_sets_2(sampler_layout);
		layouts[Const::DESCRITPOR_LAYOUT::IMAGE_SAMPLER] = sampler_layout;
		/**
		 * Init descriptor set layout for texture bucket.
		 */
		VkDescriptorSetLayout bucket_layout = texture_system.make_bucket_descriptor_set_layout();
		texture_bucket_descriptor_sets = texture_system.make_bucket_descriptor_sets_2(bucket_layout);
		font_bucket_descriptor_sets = font_system.texture_system.make_bucket_descriptor_sets_2(bucket_layout);
		layouts[Const::DESCRITPOR_LAYOUT::BUCKET_IMAGE_SAMPLER] = bucket_layout;
		/**
		 * Init descriptor set layout uniform buffer.
		 */
		layout_builder.clear();
		layout_builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
		VkDescriptorSetLayout uniform_layout = layout_builder.build(device);
		layouts[Const::DESCRITPOR_LAYOUT::UNIFORM_BUFFER] = uniform_layout;
		/**
		 * Init descriptor set layout empty.
		 */
		layout_builder.clear();
		layouts[Const::DESCRITPOR_LAYOUT::EMPTY] = layout_builder.build(device);
	}

	void destroy_common_descriptor_set_layout() {
		for (const auto& [type, layout] : layouts) {
			vkDestroyDescriptorSetLayout(device, layout, nullptr);
		}
	}

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

	bool is_using_instance_material(Const::DRAW_ID draw_type) {
		switch (draw_type) {
			case Const::DRAW_ID::DRAW_MODEL_3D: {
				return true;
			}
			default: {
				return false;
			}
		}
	}

	void sort_draws() {
		Profiler::start_scope(SCOPE_SORT_DRAW);
		sorted_draws.clear();
		auto draw_ids = draw_id_to_info.keys();
		for (auto draw_id : draw_ids) {
			const auto& draw_info = draw_id_to_info.get(draw_id);
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

	void batching_draw_groups_2(VkCommandBuffer command_buffer, uint32_t frame_index) {
		Profiler::start_scope(SCOPE_BATCHING_GROUP);
		if (sorted_draws.size() <= 0) {
			return;
		}
		draw_groups.clear();
		uint32_t first_id = sorted_draws[0].draw_id;
		const Draw_Information& first_draw = draw_id_to_info.get(first_id);
		draw_groups.push_back({first_draw.draw_type, 0, 1, first_draw.draw_material_id});
		uint32_t current_group = 0;
		uint32_t current_group_draw = 0;
		for (uint32_t i = 1; i < sorted_draws.size(); i++) {
			uint32_t last_draw_id = sorted_draws[current_group_draw].draw_id;
			uint32_t draw_id = sorted_draws[i].draw_id;
			const Draw_Information& last_draw = draw_id_to_info.get(last_draw_id);
			const Draw_Information& current_draw = draw_id_to_info.get(draw_id);
			if (is_same_draw(last_draw, current_draw)) {
				draw_groups[current_group].number_element++;
			} else {
				draw_groups.push_back({current_draw.draw_type, i, 1, current_draw.draw_material_id});
				current_group_draw = i;
				current_group++;
			}
		}
		draw_instances.clear();
		draw_instance_batching_groups.clear();
		uint32_t current_offset = 0;
		for (const Draw_Group_Batching& draw_group : draw_groups) {
			uint32_t start = draw_group.start_index;
			uint32_t end = draw_group.start_index + draw_group.number_element;
			uint32_t instance_size = (uint32_t)get_instance_size(draw_group.draw_type);
			uint32_t remain_size = Utils::calculate_range_remain_size(current_offset, instance_size);
			if (is_using_instance_material(draw_group.draw_type)) {
				draw_instance_in_groups.clear();
				for (int i = start; i < end; i++) {
					uint32_t draw_id = sorted_draws[i].draw_id;
					const auto& instances = draw_id_to_instance.get(draw_id);
					for (const auto& draw_instance : instances) {
						draw_instance_in_groups.push_back(draw_instance);
					}
				}
				std::sort(draw_instance_in_groups.begin(), draw_instance_in_groups.end());
				const Draw_Instance_Information& first_instance = draw_instance_in_groups[0];
				draw_instance_batching_groups.push_back(
					Draw_Group_Instance_Batching{
						draw_group.draw_type, draw_group.draw_material_id, first_instance.instance_material_id,
						current_offset + remain_size, first_instance.number_sub_instance
					}
				);
				current_offset += remain_size + first_instance.number_sub_instance;
				uint32_t current_group = 0;
				uint32_t current_group_instance = 0;
				for (int i = 1; i < draw_instance_in_groups.size(); i++) {
					const Draw_Instance_Information& last_instance = draw_instance_in_groups[current_group_instance];
					const Draw_Instance_Information& current_instance = draw_instance_in_groups[i];
					if (current_instance.instance_material_id == last_instance.instance_material_id) {
						draw_instance_batching_groups[current_group].number_instance +=
							current_instance.number_sub_instance;
					} else {
						draw_instance_batching_groups.push_back(
							Draw_Group_Instance_Batching{
								draw_group.draw_type, draw_group.draw_material_id,
								current_instance.instance_material_id, current_offset + remain_size,
								current_instance.number_sub_instance
							}
						);
						current_group++;
						current_group_instance = i;
					}
					current_offset += remain_size + instance_size * current_instance.number_sub_instance;
				}
				for (const auto& draw_instance : draw_instance_in_groups) {
					draw_instances.push_back(draw_instance);
				}
			} else {
				uint32_t number_instance = 0;
				for (int i = start; i < end; i++) {
					uint32_t draw_id = sorted_draws[i].draw_id;
					const auto& instances = draw_id_to_instance.get(draw_id);
					for (const auto& draw_instance : instances) {
						draw_instances.push_back(draw_instance);
						number_instance += draw_instance.number_sub_instance;
					}
				}
				uint32_t start_group_draw_id = sorted_draws[start].draw_id;
				const auto& instances = draw_id_to_instance.get(start_group_draw_id);
				draw_instance_batching_groups.push_back(
					Draw_Group_Instance_Batching{
						draw_group.draw_type, draw_group.draw_material_id, instances[0].instance_material_id,
						current_offset + remain_size, number_instance
					}
				);
				current_offset += remain_size + number_instance * instance_size;
			}
		}
		SSBO_Buffer& ssbo = get_ssbo();
		instances_copy_data.clear();
		current_offset = 0;
		for (uint32_t i = 0; i < draw_instances.size(); i++) {
			const Draw_Instance_Information& instance_info = draw_instances[i];
			const Buffer_Range& instance_range = ssbo.view_slot(instance_info.instance_id);
			uint32_t instance_size = instance_range.inner_size() / instance_info.number_sub_instance;
			uint32_t remain_size = Utils::calculate_range_remain_size(current_offset, instance_size);
			instances_copy_data.push_back(
				{(VkDeviceSize)instance_range.inner_offset(), (VkDeviceSize)current_offset + remain_size,
				 (VkDeviceSize)instance_range.inner_size()}
			);
			current_offset += remain_size + instance_range.inner_size();
		}
		Buffer& instance_buffer = get_instance_buffer(frame_index);
		Resize_Buffer_Synchronize_Information& resize_info = frame_resize_instance_buffer[current_frame];
		if (resize_info.is_resizing) {
			resize_info.old_buffer.destroy();
			resize_info.is_resizing = false;
		}
		if (instance_buffer.size < current_offset) {
			uint32_t size = (uint32_t)(current_offset * 1.5f);
			resize_info.old_buffer = instance_buffer.record_resize(command_buffer, size);
			resize_info.is_resizing = true;
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
		Profiler::end_scope(SCOPE_BATCHING_GROUP);
	}

	Buffer& get_instance_buffer(uint32_t frame_index) {
		return instance_buffers[frame_index];
	}

	const VkDescriptorSetLayout& get_layout(Const::DESCRITPOR_LAYOUT type) {
		return layouts[type];
	}

	uint32_t load_texture(std::string texture_path) {
		uint32_t texture_id = texture_system.load_texture(texture_path, Const::ENABLED_TEXTURE_BUCKETS);
		Texture_View texture_view = texture_system.view_texture(texture_id);
		if (texture_view.storage_mode == Const::TEXTURE_STORAGE_MODE::BUCKET) {
			return texture_id;
		}
		if (!frame_texture_descriptor_sets.has(texture_id)) {
			std::array<VkDescriptorSet, Const::MAX_FRAMES_IN_FLIGHT> texture_descriptor_sets =
				Utils::make_texture_descriptor_sets_2(
					descriptor_pools_2, layouts[Const::DESCRITPOR_LAYOUT::IMAGE_SAMPLER],
					texture_view.image.get_descriptor_info(), device
				);
			frame_texture_descriptor_sets.insert(texture_id, texture_descriptor_sets);
		}
		return texture_id;
	}

	const Texture_View& view_texture(uint32_t texture_id) {
		return texture_system.view_texture(texture_id);
	}

	uint32_t load_font(std::string font_path) {
		uint32_t font_id = font_system.load_font(font_path);
		const Font& font = font_system.view_font(font_id);
		Texture_View texture_view = font_system.texture_system.view_texture(font.texture_id);
		if (texture_view.storage_mode == Const::TEXTURE_STORAGE_MODE::BUCKET) {
			return font_id;
		}
		if (!frame_font_descriptor_sets.has(font.texture_id)) {
			std::array<VkDescriptorSet, Const::MAX_FRAMES_IN_FLIGHT> font_descriptor_sets =
				Utils::make_texture_descriptor_sets_2(
					descriptor_pools_2, layouts[Const::DESCRITPOR_LAYOUT::IMAGE_SAMPLER],
					texture_view.image.get_descriptor_info(), device
				);
			frame_font_descriptor_sets.insert(font.texture_id, font_descriptor_sets);
		}
		return font_id;
	}

	const Font& view_font(uint32_t font_id) {
		return font_system.view_font(font_id);
	}

	const Texture_View& view_font_texture(uint32_t font_id) {
		const Font& font = view_font(font_id);
		return font_system.texture_system.view_texture(font.texture_id);
	}

	uint32_t load_model(std::string path) {
		uint32_t model_id = model_3D_system.load_model(path);
		const auto& model = view_model(model_id);
		for (const auto& mesh : model.meshes) {
			for (const auto& primitive : mesh.primitives) {
				Texture_View texture_view = view_texture(primitive.texture_id);
				if (texture_view.storage_mode == Const::TEXTURE_STORAGE_MODE::INDIVIDUAL) {
					if (!frame_texture_descriptor_sets.has(primitive.texture_id)) {
						std::array<VkDescriptorSet, Const::MAX_FRAMES_IN_FLIGHT> texture_descriptor_sets =
							Utils::make_texture_descriptor_sets_2(
								descriptor_pools_2, layouts[Const::DESCRITPOR_LAYOUT::IMAGE_SAMPLER],
								texture_view.image.get_descriptor_info(), device
							);
						frame_texture_descriptor_sets.insert(primitive.texture_id, texture_descriptor_sets);
					}
				}
			}
		}
		return model_id;
	}

	const Model_Information& view_model(uint32_t model_id) {
		return model_3D_system.view_model(model_id);
	}

	uint32_t get_primitive_transform_offset(
		uint32_t model_id, uint32_t scene_index, uint32_t mesh_index, uint32_t primitive_index
	) {
		const Model_Information& model = view_model(model_id);
		const auto& mesh = model.meshes[mesh_index];
		uint32_t ssbo_transform_id = mesh.local_transform_storage_ids_by_scene[scene_index];
		SSBO_Buffer& ssbo = get_ssbo();
		const Buffer_Range& transform_range = ssbo.view_slot(ssbo_transform_id);
		return transform_range.offset_as<glm::mat4>() + primitive_index;
	}

	VkDescriptorSet get_storage_descriptor_set(uint32_t frame_index) {
		return storage_buffer_descriptor_sets[frame_index];
	}

	std::array<VkDescriptorSet, 2> get_image_descriptor_from(
		Texture_System& texture_system, uint32_t texture_id, uint32_t frame_index,
		const Sparse_Set<std::array<VkDescriptorSet, Const::MAX_FRAMES_IN_FLIGHT>>& frame_texture_descriptor_sets,
		const std::array<VkDescriptorSet, Const::MAX_FRAMES_IN_FLIGHT>& default_texture_descriptor_sets,
		const std::array<VkDescriptorSet, Const::MAX_FRAMES_IN_FLIGHT>& texture_bucket_descriptor_sets
	) {
		VkDescriptorSet texture_bucket = texture_bucket_descriptor_sets[frame_index];
		Texture_View texture_view = texture_system.view_texture(texture_id);
		if (texture_view.storage_mode == Const::TEXTURE_STORAGE_MODE::BUCKET) {
			return {default_texture_descriptor_sets[frame_index], texture_bucket};
		}
		if (!frame_texture_descriptor_sets.has(texture_id)) {
			return {default_texture_descriptor_sets[frame_index], texture_bucket};
		}
		const auto& texture_descriptor_sets = frame_texture_descriptor_sets.get(texture_id);
		return {texture_descriptor_sets[frame_index], texture_bucket};
	}

	std::array<VkDescriptorSet, 2> get_descriptor_set_texture_sampler(uint32_t texture_id, uint32_t frame_index) {
		return get_image_descriptor_from(
			texture_system, texture_id, frame_index, frame_texture_descriptor_sets, default_texture_descriptor_sets,
			texture_bucket_descriptor_sets
		);
	}

	std::array<VkDescriptorSet, 2> get_descriptor_set_font_sampler(uint32_t font_id, uint32_t frame_index) {
		const Font& font = font_system.view_font(font_id);
		return get_image_descriptor_from(
			font_system.texture_system, font.texture_id, frame_index, frame_font_descriptor_sets,
			default_font_descriptor_sets, font_bucket_descriptor_sets
		);
	}

	void update_storage_buffer_descriptor_sets(uint32_t frame_index) {
		SSBO_Buffer& ssbo = get_ssbo();
		bool is_storage_buffer_update = ssbo.is_storage_buffer_resizing(frame_index);
		Buffer& storage_buffer = ssbo.storage_buffers[frame_index];
		Descriptor_Set_Writer writer{};
		for (int i = 0; i < Const::MAX_DESCRIPTOR_STORAGE_BUFFER_BINDING; i++) {
			writer
				.add_buffer_write(
					i, &storage_buffer.descriptor, storage_buffer_descriptor_sets[frame_index],
					VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
				)
				.write(device);
			writer.clear();
		}
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
			std::vector<VkDescriptorSet>& binding_descriptor_sets = current_binding_draw_info.binding_descriptor_sets;
			for (int i = 0; i < binding_descriptor_sets.size(); i++) {
				binding_descriptor_sets[i] = VK_NULL_HANDLE;
			}
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
		init_common_descriptor_set_layout();
		for (int i = 0; i < instance_buffers.size(); i++) {
			Buffer& instance = instance_buffers[i];
			instance.make_buffer(
				Const::INITIALIZE_SIZE_INSTANCING_BUFFER, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physical_device, device
			);
		}
		/**
		 * Initialize all draw type.
		 */
		Texture_2D::init();
		Rectangle::init();
		Triangle::init();
		Font_2D::init();
		// Model_3D::init();
	}

	void setup_draw() {
		// Model_3D::setup_draw(current_frame);
	}

	void draw(VkCommandBuffer command_buffer) {
		update_storage_buffer_descriptor_sets(current_frame);
		Profiler::start_scope(SCOPE_DRAW);
		current_binding_draw_info.clean();
		for (const auto& group : draw_instance_batching_groups) {
			switch (group.draw_type) {
				case Const::DRAW_ID::DRAW_TEXTURE_2D: {
					Texture_2D::draw(
						command_buffer, group.draw_material_id, group.number_instance, group.offset, current_frame
					);
					break;
				}
				case Const::DRAW_ID::DRAW_FONT_2D: {
					Font_2D::draw(
						command_buffer, group.draw_material_id, group.number_instance, group.offset, current_frame
					);
					break;
				}
			}
		}
		// for (const Group_Draw_Batching& group : draw_groups) {
		// 	switch (group.draw_type) {
		// 		case Const::DRAW_ID::DRAW_RECTANGLE_2D: {
		// 			Rectangle::draw(
		// 				command_buffer, group.material_draw_id, group.number_draw_instance, group.instance_offset,
		// 				current_frame
		// 			);
		// 			break;
		// 		}
		// 		// case Const::DRAW_ID::DRAW_TEXTURE_2D: {
		// 		// 	Texture_2D::draw(
		// 		// 		command_buffer, group.material_draw_id, group.number_draw_instance, group.instance_offset,
		// 		// 		current_frame
		// 		// 	);
		// 		// 	break;
		// 		// }
		// 		case Const::DRAW_ID::DRAW_TRIANGLE_2D: {
		// 			Triangle::draw(
		// 				command_buffer, group.material_draw_id, group.number_draw_instance, group.instance_offset,
		// 				current_frame
		// 			);
		// 			break;
		// 		}
		// 		// case Const::DRAW_ID::DRAW_FONT_2D: {
		// 		// 	Font_2D::draw(
		// 		// 		command_buffer, group.material_draw_id, group.number_draw_instance, group.instance_offset,
		// 		// 		current_frame
		// 		// 	);
		// 		// 	break;
		// 		// }
		// 		// case Const::DRAW_ID::DRAW_MODEL_3D: {
		// 		// 	Model_3D::draw(
		// 		// 		command_buffer, group.material_draw_id, group.number_instance, group.instance_offset,
		// 		// 		current_frame
		// 		// 	);
		// 		// 	break;
		// 		// }
		// 		default: {
		// 			break;
		// 		}
		// 	}
		// }
		Profiler::end_scope(SCOPE_DRAW);
		frame_count++;
		if (frame_count % 60 == 0) {
			Profiler::view_scope(SCOPE_SORT_DRAW);
			Profiler::view_scope(SCOPE_BATCHING_GROUP);
			Profiler::view_scope(SCOPE_DRAW);
			Log::info("Number group instance drawed", draw_instance_batching_groups.size());
		}
	}

	void destroy_draw() {
		// Model_3D::destroy();
		Font_2D::destroy();
		Triangle::destroy();
		Texture_2D::destroy();
		Rectangle::destroy();
		for (const Buffer& buffer : instance_buffers) {
			buffer.destroy();
		}
		destroy_common_descriptor_set_layout();
		for (auto& resize_buffer : frame_resize_instance_buffer) {
			if (resize_buffer.is_resizing) {
				resize_buffer.old_buffer.destroy();
			}
		}
	}

	void update_draw(uint32_t id, Draw_Attribute draw_attributes) {
		if (!draw_id_to_info.has(id)) {
			return;
		}
		auto& draw_info = draw_id_to_info.modify(id);
		draw_info.draw_index = draw_attributes.draw_index;
		draw_info.visible = draw_attributes.is_visible;
	}

	uint32_t make_rectange(const Draw_Attribute& draw_attributes, Rectangle_Attributes rectangle_attributes) {
		// uint32_t draw_id = draw_id_generator.gen_id();
		// Draw_Information draw_info = Rectangle::make_draw(rectangle_attributes);
		// draw_info.draw_index = draw_attributes.draw_index;
		// draw_info.visible = draw_attributes.is_visible;
		// draw_info.create_index = ++current_create_index;
		// return draws.insert(draw_id, draw_info);
		return 0;
	}

	void update_rectangle(uint32_t id, Rectangle_Attributes rectangle_attributes) {
		// if (!draws.has(id)) {
		// 	return;
		// }
		// const Draw_Information& draw_info = draws.get(id);
		// Rectangle::update_draw(draw_info.instance_id, rectangle_attributes);
	}

	uint32_t make_texture_2D(const Draw_Attribute& draw_attributes, const Texture_2D_Attributes& texture_attributes) {
		uint32_t draw_id = draw_id_generator.gen_id();
		Draw_Information draw_info = Texture_2D::make_texture_2D(texture_attributes);
		draw_info.draw_index = draw_attributes.draw_index;
		draw_info.visible = draw_attributes.is_visible;
		draw_info.create_index = ++current_create_index;
		return draw_id_to_info.insert(draw_id, draw_info);
	}

	uint32_t make_texture_2D_2(const Draw_Attribute& draw_attributes, const Texture_2D_Attributes& texture_attributes) {
		uint32_t draw_id = draw_id_generator.gen_id();
		Draw_Information draw_info{Const::DRAW_ID::DRAW_TEXTURE_2D};
		draw_info.draw_index = draw_attributes.draw_index;
		draw_info.visible = draw_attributes.is_visible;
		std::vector<Draw_Instance_Information> instances = Texture_2D::make_texture_2D_2(draw_info, texture_attributes);
		draw_id_to_info.insert(draw_id, draw_info);
		draw_id_to_instance.insert(draw_id, instances);
		return draw_id;
	}

	void update_texture_2D(uint32_t id, const Texture_2D_Attributes& texture_attributes) {
		if (!draw_id_to_info.has(id)) {
			return;
		}
		Draw_Information& draw_info = draw_id_to_info.modify(id);
		Texture_2D::update_texture_2D(draw_info, texture_attributes);
	}

	uint32_t make_triangle(const Draw_Attribute& draw_attributes, const Triangle_Attribultes& triangle_attributes) {
		// uint32_t draw_id = draw_id_generator.gen_id();
		// Draw_Information draw_info = Triangle::make_triangle(triangle_attributes);
		// draw_info.draw_index = draw_attributes.draw_index;
		// draw_info.visible = draw_attributes.is_visible;
		// draw_info.create_index = ++current_create_index;
		// return draws.insert(draw_id, draw_info);
		return 0;
	}

	uint32_t make_font_2D(const Draw_Attribute& draw_attributes, const Font_2D_Attributes& font_attributes) {
		uint32_t draw_id = draw_id_generator.gen_id();
		Draw_Information draw_info{Const::DRAW_ID::DRAW_FONT_2D};
		draw_info.draw_index = draw_attributes.draw_index;
		draw_info.visible = draw_attributes.is_visible;
		std::vector<Draw_Instance_Information> instances = Font_2D::make_font_2D(draw_info, font_attributes);
		draw_id_to_info.insert(draw_id, draw_info);
		draw_id_to_instance.insert(draw_id, instances);
		return draw_id;
	}

	uint32_t make_model_3D(const Draw_Attribute& draw_attributes, const Model_3D_Attributes& model_attributes) {
		// uint32_t draw_id = draw_id_generator.gen_id();
		// Draw_Information draw_info = Model_3D::make_model_3D(model_attributes);
		// draw_info.draw_index = draw_attributes.draw_index;
		// draw_info.visible = draw_attributes.is_visible;
		// draw_info.create_index = ++current_create_index;
		// return draws.insert(draw_id, draw_info);
		return 0;
	}

	void update_triangle(uint32_t id, const Triangle_Attribultes& triangle_attributes) {
		// if (!draws.has(id)) {
		// 	return;
		// }
		// const Draw_Information& draw_info = draws.get(id);
		// Triangle::update_triangle(draw_info.instance_id, triangle_attributes);
	}

} // namespace Vulkan