#include <algorithm>
#include <math_custom.h>
#include <stdexcept>
#include <vulkan/vk_common.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

	void Model_Node::parse_from(Parser::Gltf_Node& node) {
		translation = std::move(node.translation);
		scale = std::move(node.scale);
		rotation = std::move(node.rotation);
		transform = std::move(node.transform);
		children_indices = std::move(node.children_indices);
		mesh_index = node.mesh_index;
		skin_index = node.skin_index;
		is_use_matrix = node.is_use_matrix;
	}

	void Model_Scene::parse_from(Parser::Gltf_Scene& scene) {
		node_root_indices = std::move(scene.node_indices);
	}

	void Model_Skin::parse_from(Parser::Gltf_Skin& skin) {
		joints = std::move(skin.joints);
		inverse_bind_poses = std::move(skin.inverse_bind_poses);
	}

	void Model_Sampler::parse_from(Parser::Gltf_Sampler& sampler) {
		number_apply = sampler.number_apply;
		if (sampler.interpolation == "LINEAR") {
			interpolation = LINEAR;
		}
		durations = std::move(sampler.durations);
		min_vec3 = std::move(sampler.min_vec3);
		max_vec3 = std::move(sampler.max_vec3);
		vec3s = std::move(sampler.vec3s);
		min_quat = std::move(sampler.min_quat);
		max_quat = std::move(sampler.max_quat);
		quats = std::move(sampler.quats);
	}

	float Model_Sampler::max_duration() const {
		if (durations.size() <= 0) {
			return 0.f;
		}
		return durations.back();
	}

	glm::vec3 Model_Sampler::sampler_vec3(float t, int& last_index) const {
		if (t < durations.front()) {
			last_index = 0;
			return vec3s.front();
		}
		if (t >= durations.back()) {
			last_index = durations.size() - 1;
			return vec3s.back();
		}
		if (last_index < 0 || durations[last_index] > t) {
			last_index = 0;
		}
		while (last_index + 1 < durations.size() - 1 && durations[last_index + 1] <= t)
			last_index++;

		if (last_index >= durations.size() - 1) {
			return vec3s.back();
		}
		int index = last_index + 1;
		float dt = durations[index] - durations[last_index];
		float factor = (durations[index] - t) / dt;
		switch (interpolation) {
			case Model_Sampler_Interpolation_Type::LINEAR: {
				return glm::mix(vec3s[index], vec3s[last_index], factor);
			}
			default: {
				return vec3s[last_index];
			}
		}
	}

	glm::quat Model_Sampler::sampler_quat(float t, int& last_index) const {
		if (t < durations.front()) {
			last_index = 0;
			return quats.front();
		}
		if (t >= durations.back()) {
			last_index = durations.size() - 1;
			return quats.back();
		}
		if (last_index < 0 || durations[last_index] > t) {
			last_index = 0;
		}
		while (last_index + 1 < durations.size() - 1 && durations[last_index + 1] <= t)
			last_index++;

		if (last_index >= durations.size() - 1) {
			return quats.back();
		}
		int index = last_index + 1;
		float dt = durations[index] - durations[last_index];
		float factor = (durations[index] - t) / dt;
		switch (interpolation) {
			case Model_Sampler_Interpolation_Type::LINEAR: {
				return glm::slerp(quats[index], quats[last_index], factor);
			}
			default: {
				return quats[last_index];
			}
		}
	}

	void Model_Channel::parse_from(Parser::Gltf_Channel& channel) {
		use_node_index = channel.use_node_index;
		sampler_index = channel.sampler_index;
		if (channel.type_sampler == "translation") {
			sampler_type = TRANSLATION;
		} else if (channel.type_sampler == "rotation") {
			sampler_type = ROTATION;
		} else if (channel.type_sampler == "scale") {
			sampler_type = SCALE;
		}
		node_index = channel.node_index;
	}

	void Model_Animation::parse_from(Parser::Gltf_Animation& animation) {
		name = animation.name;
		channels.resize(animation.channels.size());
		for (int i = 0; i < channels.size(); i++) {
			channels[i].parse_from(animation.channels[i]);
		}
		samplers.resize(animation.samplers.size());
		for (int i = 0; i < samplers.size(); i++) {
			samplers[i].parse_from(animation.samplers[i]);
		}
	}

	void Resource_Synchronize_Information::release_frame(uint32_t frame_index) {
		if (frame_index < 0 || frame_index > blocked.size())
			return;
		blocked[frame_index] = false;
	}

	bool Resource_Synchronize_Information::can_remove() {
		for (bool block : blocked) {
			if (block) {
				return false;
			}
		}
		return true;
	}

	uint32_t Buffer_Range::total_size() const {
		return align + size;
	}

	bool Buffer_Range::is_suitable_with(uint32_t size, uint32_t element_stride) const {
		element_stride = std::max(uint32_t(1), element_stride);
		uint32_t align = Utils::calculate_range_remain_size(offset, element_stride);
		return total_size() >= size + align;
	}

	glm::vec2 Texture_View::tex_scale() const {
		return {width / image.width, height / image.height};
	}

	Buffer_Range Buffer_Range::fit_with(uint32_t size, uint32_t element_stride) {
		if (!is_suitable_with(size, element_stride)) {
			throw std::runtime_error("Fail to fix static data in to current range!");
		}
		uint32_t new_align = 0;
		uint32_t new_size = size;
		element_stride = std::max(uint32_t(1), element_stride);
		new_align = Utils::calculate_range_remain_size(offset, element_stride);
		uint32_t remain_offset = offset + (new_align + new_size);
		uint32_t remain_size = total_size() - (new_align + new_size);
		this->align = new_align;
		this->size = new_size;
		this->stride = element_stride;
		return {remain_offset, 0, remain_size, 1};
	}

	uint32_t Buffer_Range::inner_offset() const {
		return offset + align;
	}

	uint32_t Buffer_Range::inner_size() const {
		return size;
	}

	bool Buffer_Range_Compare::operator()(const Buffer_Range& a, const Buffer_Range& b) const {
		return a.total_size() > b.total_size();
	}

	char Font::to_char_index(char value) const {
		uint32_t char_index = (uint32_t)(value - start_char);
		if (char_index >= rects.size()) {
			char_index = (uint32_t)('*' - start_char);
		}
		return char_index;
	}

	glm::vec2 Font::get_char_bounding_box(char value) const {
		uint32_t char_index = to_char_index(value);
		const auto& rect = rects[char_index];
		return {rect.bounding_width, line_height};
	}

	glm::vec2 Font::get_char_local_offset(char value) const {
		uint32_t char_index = to_char_index(value);
		const auto& rect = rects[char_index];
		return {rect.offset_x, rect.offset_y};
	}

	float Font::get_char_scale(int font_size) const {
		font_size = std::max(std::min(font_size, max_font_size), min_font_size);
		return (float)font_size / max_font_size;
	}

	uint32_t Font::get_char_rect_offset(char value) const {
		uint32_t char_index = to_char_index(value);
		return rect_storage_offset + char_index;
	}

	glm::mat4 Model_3D_Attributes::get_transform() const {
		return Math::make_translation(position) * Math::make_rotation(rotation) * Math::make_scale(scale);
	}

	glm::vec3 Model_Mesh::get_mesh_origin() const {
		return (max_bounding_box + min_bounding_box) * 0.5f;
	}

	glm::mat4 Model_Node::get_transform() const {
		return Math::make_translation(translation) * glm::mat4_cast(rotation) * Math::make_scale(scale) * transform;
	}

	int Model::find_scene_index_by(const std::string& name) const {
		for (int i = 0; i < scenes.size(); i++) {
			if (scenes[i].name == name) {
				return i;
			}
		}
		return default_scene_index;
	}

	int Model::find_animation_index_by(const std::string& name) const {
		for (int i = 0; i < animations.size(); i++) {
			if (animations[i].name == name) {
				return i;
			}
		}
		return -1;
	}

	void Model::dfs_write(int scene_index, std::function<void(Model&, int, int, int)> execute) {
		if (scene_index == -1) {
			scene_index = default_scene_index;
		}
		std::stack<uint32_t> stack{};
		std::vector<bool> visited(nodes.size());
		for (uint32_t root : scenes[scene_index].node_root_indices) {
			for (int i = 0; i < visited.size(); i++) {
				visited[i] = false;
			}
			stack.push(root);
			visited[root] = true;
			execute(*this, scene_index, -1, (int)root);
			while (stack.size()) {
				uint32_t parent = stack.top();
				stack.pop();
				const auto& children_indices = nodes[parent].children_indices;
				for (int i = 0; i < children_indices.size(); i++) {
					uint32_t children = children_indices[i];
					if (!visited[children]) {
						stack.push(children);
						visited[children] = true;
						execute(*this, scene_index, (int)parent, (int)children);
					}
				}
			}
		}
	}

	void Model::dfs_read(int scene_index, std::function<void(const Model&, int, int, int)> execute) const {
		if (scene_index == -1) {
			scene_index = default_scene_index;
		}
		std::stack<uint32_t> stack{};
		std::vector<bool> visited(nodes.size());
		for (uint32_t root : scenes[scene_index].node_root_indices) {
			for (int i = 0; i < visited.size(); i++) {
				visited[i] = false;
			}
			stack.push(root);
			visited[root] = true;
			execute(*this, scene_index, -1, (int)root);
			while (stack.size()) {
				uint32_t parent = stack.top();
				stack.pop();
				const auto& children_indices = nodes[parent].children_indices;
				for (int i = 0; i < children_indices.size(); i++) {
					uint32_t children = children_indices[i];
					if (!visited[children]) {
						stack.push(children);
						visited[children] = true;
						execute(*this, scene_index, (int)parent, (int)children);
					}
				}
			}
		}
	}
} // namespace Vulkan