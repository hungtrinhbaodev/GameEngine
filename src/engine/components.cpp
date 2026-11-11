#include <algorithm>
#include <engine/components.h>
#include <math_custom.h>
#include <string_view>
#include <vulkan/draws/vk_draw_font_2D.h>

namespace Engine {

	bool Size_Pinning::is_changing(const Size_Pinning_Data& pinning) const {
		return left != pinning.left && right != pinning.right && top != pinning.top && bottom != pinning.bottom;
	}

	void Size_Pinning::update(const Size_Pinning_Data& pinning) {
		top = pinning.top;
		bottom = pinning.bottom;
		left = pinning.left;
		right = pinning.right;
	}

	Size_Pinning_Data Size_Pinning::get_size_pinning() const {
		Size_Pinning_Data data{};
		data.top = top;
		data.bottom = bottom;
		data.left = left;
		data.right = right;
		return data;
	}

	void Dirty_Flag::add_flag(uint64_t flag) {
		value |= flag;
	}

	void Dirty_Flag::clean_flag(uint64_t flag) {
		value &= ~flag;
	}

	bool Dirty_Flag::is_dirty_at(uint64_t flag) const {
		return value & flag;
	}

	void Clipping_Ancestor_Information::update_self_clipping(const Handle& e) {
		self_clipping = e;
	}

	void Clipping_Ancestor_Information::merge_with(
		const Clipping_Ancestor_Information& parent_clipping_ancestor, bool is_parent_clipping,
		const Handle& parent_handle
	) {
		clipping_ancestors = parent_clipping_ancestor.clipping_ancestors;
		number_clipping = parent_clipping_ancestor.number_clipping;
		if (is_parent_clipping) {
			add_clipping_ancestor(parent_handle);
		}
	}

	void Clipping_Ancestor_Information::add_clipping_ancestor(const Handle& ancestor) {
		if (::Const::MAX_DEPTH_CLIPPING <= 0)
			return;
		bool need_remove = false;
		for (int i = 0; i < number_clipping; i++) {
			if (clipping_ancestors[i].id() == ancestor.id()) {
				need_remove = true;
				break;
			}
		}
		if (need_remove) {
			remove_clipping_ancestor(ancestor);
		}
		if (number_clipping >= ::Const::MAX_DEPTH_CLIPPING) {
			remove_clipping_ancestor(clipping_ancestors[0]);
		}
		clipping_ancestors[number_clipping++] = ancestor;
	}

	void Clipping_Ancestor_Information::remove_clipping_ancestor(const Handle& ancestor) {
		if (::Const::MAX_DEPTH_CLIPPING <= 0)
			return;
		int remove_index = -1;
		for (int i = 0; i < ::Const::MAX_DEPTH_CLIPPING; i++) {
			if (clipping_ancestors[i].id() == ancestor.id()) {
				remove_index = i;
				break;
			}
		}
		if (remove_index < 0)
			return;
		for (int i = remove_index + 1; i < ::Const::MAX_DEPTH_CLIPPING; i++) {
			clipping_ancestors[i - 1] = clipping_ancestors[i];
		}
		number_clipping--;
	}

	std::tuple<int, std::array<Handle, ::Const::MAX_DEPTH_CLIPPING>> Clipping_Ancestor_Information::
		get_self_clipping_info() const {
		std::array<Handle, ::Const::MAX_DEPTH_CLIPPING> handles;
		if (self_clipping.is_invalid()) {
			return std::make_tuple(0, handles);
		} else {
			handles[0] = self_clipping;
			return std::make_tuple(1, handles);
		}
	}

	std::tuple<int, std::array<Handle, ::Const::MAX_DEPTH_CLIPPING>> Clipping_Ancestor_Information::
		get_clipping_ancestors_info() const {
		if (self_clipping.is_invalid()) {
			return std::make_tuple(number_clipping, clipping_ancestors);
		}
		int total_number_clipping = number_clipping;
		auto final_clipping_ancestors = clipping_ancestors;
		if (total_number_clipping >= ::Const::MAX_DEPTH_CLIPPING) {
			for (int i = 0; i < final_clipping_ancestors.size() - 1; i++) {
				final_clipping_ancestors[i] = final_clipping_ancestors[i + 1];
			}
			final_clipping_ancestors[total_number_clipping - 1] = self_clipping;
		} else {
			final_clipping_ancestors[total_number_clipping++] = self_clipping;
		}
		return std::make_tuple(total_number_clipping, final_clipping_ancestors);
	}

	World_Transform_2D World_Transform_2D::transform(const Transform_2D& local_transform) const {
		World_Transform_2D world_transform{};
		world_transform.position = position + Math::rotate(rotation, local_transform.position * scale);
		world_transform.scale = scale * local_transform.scale;
		world_transform.rotation = rotation + local_transform.rotation;
		world_transform.color = local_transform.color;
		world_transform.opacity = local_transform.opacity;
		world_transform.anchor = local_transform.anchor;
		world_transform.is_visible = is_visible & local_transform.is_visible;
		return world_transform;
	}

	void Label_Renderer_Information::update_inner_text_by(const glm::vec2& text_size) {
		if (!use_custom_size)
			return;
		auto fit_transform = [this](const glm::vec2& text_size, const glm::vec2& inner_size, const glm::vec2& padding) {
			glm::vec2 label_position = {0.f, 0.f};
			switch (horizontal_align) {
				case LABEL_HORIZONTAL_ALIGN::LABEL_HORIZONTAL_ALIGN_CENTER: {
					label_position.x = (text_size.x - inner_size.x) / 2;
					break;
				}
				case LABEL_HORIZONTAL_ALIGN::LABEL_HORIZONTAL_ALIGN_LEFT: {
					label_position.x = padding.x;
					break;
				}
				default: {
					label_position.x = text_size.x - padding.x - inner_size.x;
					break;
				}
			}
			switch (vertical_align) {
				case LABEL_VERTICAL_ALIGN::LABEL_VERTICAL_ALIGN_CENTER: {
					label_position.y = (text_size.y - inner_size.y) / 2;
					break;
				}
				case LABEL_VERTICAL_ALIGN::LABEL_VERTICAL_ALIGN_BOTTOM: {
					label_position.y = padding.y;
					break;
				}
				default: {
					label_position.y = text_size.y - padding.y - inner_size.y;
					break;
				}
			}
			inner_label_transform.position = label_position;
		};
		float final_horizontal_padding =
			horizontal_align == LABEL_HORIZONTAL_ALIGN::LABEL_HORIZONTAL_ALIGN_CENTER ? 0.f : horizontal_padding;
		float final_vertical_padding =
			vertical_align == LABEL_VERTICAL_ALIGN::LABEL_VERTICAL_ALIGN_CENTER ? 0.f : vertical_padding;
		glm::vec2 padding = {final_horizontal_padding, final_vertical_padding};
		glm::vec2 remain_size = text_size - padding;
		auto text_helper =
			Vulkan::Draw_Font_2D::gen_text_info_from(font_path, text, (int)horizontal_align, font_size, stroke_size);
		glm::vec2 inner_size = text_helper.get_size();
		if (inner_size.x <= remain_size.x && inner_size.y <= remain_size.y) {
			render_text = text;
			clipping_enabled = false;
			fit_transform(text_size, inner_size, padding);
			return;
		}
		auto measure = [this](std::string_view value) {
			return Vulkan::Draw_Font_2D::get_renderer_text_size(value, font_path, font_size, stroke_size);
		};
		// widths add up char by char, so every word is measured once and nothing is concatenated to measure.
		const float space_width = measure(" ").x;
		const float line_height = measure(" ").y;
		const std::string_view ellipsis = "...";
		const float ellipsis_width = measure(ellipsis).x;
		int max_lines = line_height > 0.f ? std::max(1, (int)(remain_size.y / line_height)) : 1;

		// words are views into `text` ('\n' is treated like a space); a word wider than the box is cut by chars.
		std::vector<std::string_view> words;
		std::string_view source = text;
		size_t start = 0;
		while (start < source.size()) {
			size_t end = source.find_first_of(" \n", start);
			if (end == std::string_view::npos)
				end = source.size();
			std::string_view word = source.substr(start, end - start);
			start = end + 1;
			if (word.empty())
				continue;
			if (measure(word).x <= remain_size.x) {
				words.push_back(word);
				continue;
			}
			// binary search the longest prefix of the rest that fits the width (at least 1 char, so it always ends).
			std::string_view rest = word;
			while (!rest.empty()) {
				size_t low = 1, high = rest.size();
				while (low < high) {
					size_t mid = (low + high + 1) / 2;
					if (measure(rest.substr(0, mid)).x <= remain_size.x) {
						low = mid;
					} else {
						high = mid - 1;
					}
				}
				words.push_back(rest.substr(0, low));
				rest.remove_prefix(low);
			}
		}

		// greedy fill: keep a running width per line instead of re-measuring line + " " + word.
		std::vector<std::string> lines;
		std::vector<float> line_widths;
		bool truncated = false;
		for (std::string_view word : words) {
			float word_width = measure(word).x;
			if (!lines.empty() && line_widths.back() + space_width + word_width <= remain_size.x) {
				lines.back() += ' ';
				lines.back() += word;
				line_widths.back() += space_width + word_width;
				continue;
			}
			if (fit_mode == LABEL_RENDER_FIT_MODE::ELLIPSIS && (int)lines.size() >= max_lines) {
				truncated = true;
				break;
			}
			lines.emplace_back(word);
			line_widths.push_back(word_width);
		}
		if (truncated && !lines.empty()) {
			std::string& last_line = lines.back();
			float last_width = line_widths.back();
			while (!last_line.empty() && last_width + ellipsis_width > remain_size.x) {
				last_width -= measure(std::string_view(last_line).substr(last_line.size() - 1)).x;
				last_line.pop_back();
			}
			last_line += ellipsis;
		}

		render_text.clear();
		for (size_t i = 0; i < lines.size(); i++) {
			if (i > 0)
				render_text += '\n';
			render_text += lines[i];
		}
		text_helper = Vulkan::Draw_Font_2D::gen_text_info_from(
			font_path, render_text, (int)horizontal_align, font_size, stroke_size
		);
		fit_transform(text_size, text_helper.get_size(), padding);
		switch (fit_mode) {
			case LABEL_RENDER_FIT_MODE::CLIP: {
				clipping_enabled = true;
				break;
			}
			default: {
				clipping_enabled = text_helper.width > remain_size.x || text_helper.height > remain_size.y;
				break;
			}
		}
	}

	void Nine_Scale_Information::update_piece_transforms(
		const glm::vec2& renderer_size, const World_Transform_2D& world_transform, const glm::vec2& size,
		const Geometry::Texture_Rect_2D& rect
	) {
		if (size.x <= 0 || size.y <= 0 || renderer_size.x <= 0 || renderer_size.y <= 0) {
			for (int i = 0; i < pieces_transforms.size(); i++) {
				pieces_transforms[i].is_visible = false;
			}
			return;
		}
		if (!use_rect_ratio) {
			capset_rect.x = std::min(capset_rect.x, size.x);
			capset_rect.y = std::min(capset_rect.y, size.y);
			capset_rect.width = std::min(capset_rect.width, size.x - capset_rect.x);
			capset_rect.height = std::min(capset_rect.height, size.y - capset_rect.y);

			capset_rect_ratio.ratio_x = capset_rect.x / size.x;
			capset_rect_ratio.ratio_y = capset_rect.y / size.y;
			capset_rect_ratio.ratio_width = capset_rect.width / size.x;
			capset_rect_ratio.ratio_height = capset_rect.height / size.y;
		}
		/**
		 * bottom-left, bottom-right, top-right, top-left
		 */
		std::array<glm::vec2, 4> corner_ratios{
			glm::vec2(capset_rect_ratio.ratio_x, capset_rect_ratio.ratio_y),
			glm::vec2(
				1.f - (capset_rect_ratio.ratio_x + capset_rect_ratio.ratio_width), capset_rect_ratio.ratio_height
			),
			glm::vec2(
				1.f - (capset_rect_ratio.ratio_x + capset_rect_ratio.ratio_width),
				1.f - (capset_rect_ratio.ratio_y + capset_rect_ratio.ratio_height)
			),
			glm::vec2(capset_rect_ratio.ratio_x, 1.f - (capset_rect_ratio.ratio_y + capset_rect_ratio.ratio_height))
		};
		std::array<glm::vec2, 4> corner_sizes{
			corner_ratios[0] * size, corner_ratios[1] * size, corner_ratios[2] * size, corner_ratios[3] * size
		};
		if (renderer_size.x < corner_sizes[0].x + corner_sizes[1].x ||
			renderer_size.y < corner_sizes[0].y + corner_sizes[3].y || capset_rect_ratio.ratio_width <= 0 ||
			capset_rect_ratio.ratio_height <= 0 || size.x <= 0 || size.y <= 0) {
			pieces_transforms[0] = world_transform;
			pieces_transforms[0].scale *= renderer_size / size;
			pieces_rects[0] = rect;
			for (int i = 1; i < pieces_transforms.size(); i++) {
				pieces_transforms[i].is_visible = false;
			}
			return;
		}
		glm::vec2 size_pieces_scale = size * glm::vec2(capset_rect_ratio.ratio_width, capset_rect_ratio.ratio_height);
		glm::vec2 remain_size{};
		remain_size.x = renderer_size.x - (corner_sizes[0].x + corner_sizes[1].x);
		remain_size.y = renderer_size.y - (corner_sizes[0].y + corner_sizes[3].y);
		glm::vec2 scale_pieces = glm::vec2(remain_size.x / size_pieces_scale.x, remain_size.y / size_pieces_scale.y);
		auto get_pieces_index = [](int row, int col) { return row * 3 + col; };
		auto get_pieces_rect_ratio = [this, &corner_ratios, &rect](int row, int col) {
			Geometry::Texture_Rect_2D piece_rect{};
			switch (col) {
				case 0: {
					piece_rect.ratio_x = 0;
					piece_rect.ratio_width = corner_ratios[0].x;
					break;
				}
				case 1: {
					piece_rect.ratio_x = corner_ratios[0].x;
					piece_rect.ratio_width = capset_rect_ratio.ratio_width;
					break;
				}
				default: {
					piece_rect.ratio_x = 1.f - corner_ratios[1].x;
					piece_rect.ratio_width = corner_ratios[1].x;
					break;
				}
			}
			switch (row) {
				case 0: {
					piece_rect.ratio_y = 0;
					piece_rect.ratio_height = corner_ratios[0].y;
					break;
				}
				case 1: {
					piece_rect.ratio_y = corner_ratios[0].y;
					piece_rect.ratio_height = capset_rect_ratio.ratio_height;
					break;
				}
				default: {
					piece_rect.ratio_y = 1.f - corner_ratios[3].y;
					piece_rect.ratio_height = corner_ratios[3].y;
					break;
				}
			}
			piece_rect.ratio_width = piece_rect.ratio_width * rect.ratio_width;
			piece_rect.ratio_x = rect.ratio_x + rect.ratio_width * piece_rect.ratio_x;
			piece_rect.ratio_height = piece_rect.ratio_height * rect.ratio_height;
			piece_rect.ratio_y = rect.ratio_y + rect.ratio_height * piece_rect.ratio_y;
			return piece_rect;
		};
		auto get_pieces_scale = [&scale_pieces](int row, int col) {
			glm::vec2 scale{1.f, 1.f};
			switch (col) {
				case 1: {
					scale.x = scale_pieces.x;
					break;
				}
				default: {
					scale.x = 1.f;
					break;
				}
			}
			switch (row) {
				case 1: {
					scale.y = scale_pieces.y;
					break;
				}
				default: {
					scale.y = 1.f;
					break;
				}
			}
			return scale;
		};
		float y = 0.f;
		glm::vec2 anchor{0.f, 0.f};
		float rotation = 0.f;
		glm::vec2 local_origin = world_transform.anchor * renderer_size;
		for (int i = 0; i < 3; i++) {
			float x = 0.f;
			for (int j = 0; j < 3; j++) {
				int index = get_pieces_index(i, j);
				pieces_rects[index] = get_pieces_rect_ratio(i, j);
				glm::vec2 scale = get_pieces_scale(i, j);
				Transform_2D local_transform = world_transform;
				local_transform.anchor = anchor;
				local_transform.scale = scale;
				local_transform.rotation = rotation;
				local_transform.position = glm::vec2(x, y) - local_origin;
				pieces_transforms[index] = world_transform.transform(local_transform);
				switch (j) {
					case 0: {
						x += corner_sizes[0].x;
						break;
					}
					case 1: {
						x += remain_size.x;
						break;
					}
				}
			}
			switch (i) {
				case 0: {
					y += corner_sizes[0].y;
					break;
				}
				case 1: {
					y += remain_size.y;
					break;
				}
			}
		}
	}

	bool Rect::is_clipped_by(const Rect& parent_rect) const {
		for (int i = 0; i < parent_rect.transformed_rect.size(); i++) {
			if (Math::point_in_rect(transformed_rect, parent_rect.transformed_rect[i])) {
				return false;
			}
		}
		for (int i = 0; i < transformed_rect.size(); i++) {
			if (Math::point_in_rect(parent_rect.transformed_rect, transformed_rect[i])) {
				return false;
			}
		}
		return true;
	}

	CLIPPING_STATE Rect::get_cliping_state_with(const Rect& parent_rect) const {
		switch (Math::get_collision_state(transformed_rect, parent_rect.transformed_rect)) {
			case ::Const::COLLISION_STATE_FULL_IN: {
				return CLIPPING_FULL_IN;
			}
			case ::Const::COLLISION_STATE_FULL_OUT: {
				return CLIPPING_FULL_OUT;
			}
			default: {
				return CLIPPING_INTERECT;
			}
		}
	}

} // namespace Engine