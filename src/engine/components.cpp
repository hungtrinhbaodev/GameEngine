#include <algorithm>
#include <engine/components.h>
#include <engine/utils.h>
#include <math_custom.h>
#include <string_view>
#include <utf8_helper.h>
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

	void Clipping_Ancestor_Information::clean_up() {
		self_clipping = Handle::make_invalid_handle();
		number_clipping = 0;
		clipping_ancestors = {};
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

	void Image_Renderer_Information::update_transform_inner_image(
		const glm::vec2& size, const glm::vec2& self_scale, const World_Transform_2D& world_transform
	) {
		inner_image_transform.anchor = glm::vec2{0.5f, 0.5f};
		inner_image_transform.position = (glm::vec2{0.5f, 0.5f} - world_transform.anchor) * size;
		inner_image_transform.scale = self_scale;
		inner_image_transform.is_visible = world_transform.is_visible;
		inner_image_transform.color = world_transform.color;
		inner_image_transform.opacity = world_transform.opacity;
	}

	glm::vec2 Label_Renderer_Information::get_padding_size() const {
		float final_horizontal_padding =
			horizontal_align == LABEL_HORIZONTAL_ALIGN::LABEL_HORIZONTAL_ALIGN_CENTER ? 0.f : horizontal_padding;
		float final_vertical_padding =
			vertical_align == LABEL_VERTICAL_ALIGN::LABEL_VERTICAL_ALIGN_CENTER ? 0.f : vertical_padding;
		return {final_horizontal_padding, final_vertical_padding};
	}

	void Label_Renderer_Information::update_text(const std::string& i_text) {
		text = i_text;
		u32_to_utf8_bytes_start.clear();
		u32_to_utf8_bytes_length.clear();
		u32_text = Helper::utf8_to_u32_nfc_mapped(i_text, u32_to_utf8_bytes_start, u32_to_utf8_bytes_length);
		utf8_to_u32_index.resize(text.size());
		int count = 0;
		for (int i = 0; i < u32_to_utf8_bytes_start.size(); i++) {
			int start = (int)u32_to_utf8_bytes_start[i];
			int length = (int)u32_to_utf8_bytes_length[i];
			for (int j = start; j < start + length; j++) {
				utf8_to_u32_index[count] = i;
				count++;
			}
		}
	}

	/**
	 * TODO: treat \n of user is the last character of line
	 * retain the logic bounding box using flat add 2 lines
	 * rects and start and number character in line to this
	 * structure.
	 */
	void Label_Renderer_Information::update_inner_text_by(const glm::vec2& text_size, const glm::vec2& text_anchor) {
		/**
		 * @Note: [Claude BEGIN, written by Claude (Claude Code) on 2026-11-11] label wrap / ellipsis / clip
		 * for fit modes other than AUTO. Writes render_text (what is drawn), keeps `text` untouched.
		 * 1. text fits inside size - padding -> draw as is, no clip.
		 * 2. split into words (views into `text`, '\n' = space); a word wider than the box is cut with a
		 *    binary search on the longest prefix that fits (at least 1 char, so it always ends).
		 * 3. greedy fill lines with a running width (no re-measuring of whole lines).
		 * 4. ELLIPSIS: stop at max_lines and trim the last line until "..." fits.
		 * 5. measure render_text, place it with fit_transform, clip for CLIP or when it still overflows.
		 * Later edits by the user inside this block: full_path / Utils::get_full_font_path (finding 41)
		 * and the LABEL_RENDER_FIT_* enum names.
		 */
		auto update_fit_size = [this, &text_size](const glm::vec2& inner_size, const glm::vec2& padding) {
			switch (fit_mode) {
				case LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_WIDTH_SHOW_ALL: {
					fit_size = glm::vec2{text_size.x, inner_size.y + padding.y};
					break;
				}
				case LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_HEIGHT_SHOW_ALL: {
					fit_size = glm::vec2{inner_size.x + padding.x, text_size.y};
					break;
				}
				case LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_AUTO: {
					fit_size = inner_size;
					break;
				}
				default: {
					fit_size = text_size;
					break;
				}
			}
		};
		std::vector<std::vector<int>> lines_to_text_indices;
		glm::vec2 anchor{0.f};
		glm::vec2 padding = get_padding_size();
		auto text_helper = Vulkan::Draw_Font_2D::gen_text_info_from(
			font_id, u32_text, (int)horizontal_align, font_size, stroke_size, anchor
		);
		if (fit_mode == LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_AUTO) {
			render_text = u32_text;
			clipping_enabled = false;
			lines_to_text_indices.resize(text_helper.lines.size());
			int u32_index_count = 0;
			for (int i = 0; i < lines_to_text_indices.size(); i++) {
				for (int j = 0; j < text_helper.lines[i].size(); j++) {
					const auto& my_char = text_helper.lines[i][j];
					while (u32_index_count < u32_text.size()) {
						const auto& other_char = u32_text[u32_index_count];
						if (my_char == other_char) {
							break;
						}
						u32_index_count++;
					}
					lines_to_text_indices[i].push_back(u32_index_count);
					u32_index_count++;
				}
			}
			update_fit_size(text_helper.get_size(), padding);
		} else {
			glm::vec2 remain_size = text_size - padding;
			std::vector<int> character_text_indices;
			std::vector<std::vector<int>> character_in_word_indices;
			auto push_word_char_indices =
				[this, &character_in_word_indices](int start_word_index, int number_char_formed_word) {
					std::vector<int> word_indices{};
					int start = start_word_index;
					int end = start_word_index + number_char_formed_word;
					for (int i = start; i < end; i++) {
						if (i >= u32_text.size()) {
							word_indices.push_back(-1);
							continue;
						}
						word_indices.push_back(i);
					}
					character_in_word_indices.push_back(word_indices);
				};
			auto measure = [this](std::u32string_view value) {
				return Vulkan::Draw_Font_2D::get_renderer_text_size(value, font_id, font_size, stroke_size);
			};
			// widths add up char by char, so every word is measured once and nothing is concatenated to measure.
			const float space_width = measure(U" ").x;
			const float line_height = measure(U" ").y;
			const std::u32string ellipsis = U"...";
			const float ellipsis_width = measure(ellipsis).x;
			int max_lines = line_height > 0.f ? std::max(1, (int)(remain_size.y / line_height)) : 1;
			// words are views into `text` ('\n' is treated like a space); a word wider than the box is cut by
			// chars.
			std::vector<std::u32string_view> words;
			std::vector<float> word_widths;
			std::u32string_view source = u32_text;
			size_t start = 0;
			while (start < source.size()) {
				size_t end = source.find_first_of(U" \n", start);
				if (end == std::u32string_view::npos)
					end = source.size();
				std::u32string_view word = source.substr(start, end - start);
				int start_line_index = start;
				start = end + 1;
				if (word.empty()) {
					if (end != source.size()) {
						words.push_back(source.substr(end, 1));
						push_word_char_indices(end, 1);
					}
					continue;
				}
				if (measure(word).x <= remain_size.x) {
					words.push_back(word);
					push_word_char_indices(start_line_index, word.size());
					if (end != source.size()) {
						words.push_back(source.substr(end, 1));
						push_word_char_indices(end, 1);
					}
					continue;
				}
				// binary search the longest prefix of the rest that fits the width (at least 1 char, so it always
				// ends).
				std::u32string_view rest = word;
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
					push_word_char_indices(start_line_index, low);
					start_line_index += low;
					rest.remove_prefix(low);
				}
				if (end != source.size()) {
					words.push_back(source.substr(end, 1));
					push_word_char_indices(end, 1);
				}
			}
			for (const auto& word : words) {
				word_widths.push_back(measure(word).x);
			}
			std::vector<std::u32string> lines;
			auto append_word_indices_to_line = [this, &character_in_word_indices,
												&lines_to_text_indices](int word_index) {
				auto& line_indices = lines_to_text_indices.back();
				for (int i = 0; i < character_in_word_indices[word_index].size(); i++) {
					line_indices.push_back(character_in_word_indices[word_index][i]);
				}
			};
			switch (fit_mode) {
				case LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_ELLIPSIS:
				case LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_WIDTH_SHOW_ALL:
				case LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_CLIP: {
					// greedy fill: keep a running width per line instead of re-measuring line + " " + word.
					bool truncated = false;
					std::vector<float> line_widths;
					for (int i = 0; i < words.size(); i++) {
						std::u32string_view word = words[i];
						float word_width = word_widths[i];
						if (word == U"\n") {
							lines.push_back(U"");
							line_widths.push_back(0.f);
							lines_to_text_indices.push_back({});
							continue;
						}
						if (!lines.empty() && line_widths.back() + space_width + word_width <= remain_size.x) {
							lines.back() += word;
							line_widths.back() += space_width + word_width;
							append_word_indices_to_line(i);
							continue;
						}
						if (fit_mode == LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_ELLIPSIS &&
							(int)lines.size() >= max_lines) {
							truncated = true;
							break;
						}
						lines.emplace_back(word);
						line_widths.push_back(word_width);
						lines_to_text_indices.push_back({});
						append_word_indices_to_line(i);
					}
					if (truncated && !lines.empty()) {
						std::u32string& last_line = lines.back();
						auto& last_line_indices = lines_to_text_indices.back();
						float last_width = line_widths.back();
						while (!last_line.empty() && last_width + ellipsis_width > remain_size.x) {
							last_width -= measure(std::u32string_view(last_line).substr(last_line.size() - 1)).x;
							last_line.pop_back();
							last_line_indices.pop_back();
						}
						last_line += ellipsis;
						for (int i = 0; i < ellipsis.size(); i++) {
							last_line_indices.push_back(-1);
						}
					}
					break;
				}
				case LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_HEIGHT_SHOW_ALL: {
					float total_width_all_word = 0.f;
					for (int i = 0; i < words.size(); i++) {
						total_width_all_word += word_widths[i];
						if (i < (int)words.size() - 1) {
							total_width_all_word += space_width;
						}
					}
					float size_line_width = std::max(total_width_all_word / max_lines, remain_size.x);
					float current_line_width = 0.f;
					for (int i = 0; i < words.size(); i++) {
						const auto& word = words[i];
						float word_width = word_widths[i];
						if (word == U"\n") {
							lines.push_back(U"");
							size_line_width = 0.f;
							lines_to_text_indices.push_back({});
							continue;
						}
						if (lines.empty() || current_line_width + space_width + word_width > size_line_width) {
							lines.emplace_back(word);
							lines_to_text_indices.push_back({});
							append_word_indices_to_line(i);
							current_line_width = word_width;
						} else {
							current_line_width += word_width + space_width;
							append_word_indices_to_line(i);
							lines.back() += word;
						}
					}
					break;
				}
				default: {
					break;
				}
			}
			render_text.clear();
			for (size_t i = 0; i < lines.size(); i++) {
				if (i > 0)
					render_text += U'\n';
				render_text += lines[i];
			}
			text_helper = Vulkan::Draw_Font_2D::gen_text_info_from(
				font_id, render_text, (int)horizontal_align, font_size, stroke_size, anchor
			);
			update_fit_size(text_helper.get_size(), padding);
			switch (fit_mode) {
				case LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_CLIP: {
					clipping_enabled = true;
					break;
				}
				case LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_WIDTH_SHOW_ALL: {
					clipping_enabled = text_size.x > fit_size.x;
					break;
				}
				case LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_HEIGHT_SHOW_ALL: {
					clipping_enabled = text_size.y > fit_size.y;
					break;
				}
				default: {
					clipping_enabled = text_helper.width > remain_size.x || text_helper.height > remain_size.y;
					break;
				}
			}
		}
		// [Claude END] label wrap / ellipsis / clip
		render_char_rects.clear();
		number_line = 0;
		u32_to_rect_index.clear();
		rect_to_u32_index.clear();
		rect_to_line_index.clear();
		number_line = text_helper.lines.size();
		u32_to_rect_index.resize(u32_text.size(), -1);
		std::vector<glm::vec2> char_positions;
		glm::vec2 origin = -text_anchor * fit_size + padding;
		for (int i = 0; i < text_helper.lines.size(); i++) {
			const auto& line = text_helper.lines[i];
			if (line.size() <= 0)
				continue;
			const auto& start_line_position = text_helper.start_line_positions[i];
			char_positions = Vulkan::Draw_Font_2D::get_char_renderer_positions_at(
				line, start_line_position, font_id, font_size, stroke_size, false
			);
			for (int j = 0; j < line.size(); j++) {
				glm::vec2 char_position = char_positions[j];
				glm::vec2 char_bounding_box =
					Vulkan::Draw_Font_2D::get_char_render_bounding_box(line[j], font_id, font_size, stroke_size);
				glm::vec2 min = origin + char_position;
				glm::vec2 max = min + char_bounding_box;
				render_char_rects.push_back({min, max});
				rect_to_line_index.push_back(i);
				int current_rect_index = (int)render_char_rects.size() - 1;
				int current_char_index = lines_to_text_indices[i][j];
				if (current_char_index >= 0) {
					u32_to_rect_index[current_char_index] = current_rect_index;
				}
				rect_to_u32_index.push_back(current_char_index);
			}
		}
	}

	void Label_Renderer_Information::update_renderer_char_rects(
		const glm::vec2& text_anchor, const glm::vec2& text_size
	) {
		render_char_rects.clear();
		number_line = 0;
		u32_to_rect_index.clear();
		rect_to_u32_index.clear();
		rect_to_line_index.clear();
		glm::vec2 inner_anchor = {0.f, 0.f};
		auto text_helper = Vulkan::Draw_Font_2D::gen_text_info_from(
			font_id, render_text, (int)horizontal_align, font_size, stroke_size, inner_anchor
		);
		number_line = text_helper.lines.size();
		std::u32string flat_text = U"";
		std::vector<glm::vec2> char_positions;
		glm::vec2 padding = get_padding_size();
		glm::vec2 origin = -text_anchor * text_size + padding;
		for (int i = 0; i < text_helper.lines.size(); i++) {
			const auto& line = text_helper.lines[i];
			const auto& start_line_position = text_helper.start_line_positions[i];
			char_positions = Vulkan::Draw_Font_2D::get_char_renderer_positions_at(
				line, start_line_position, font_id, font_size, stroke_size, false
			);
			for (int j = 0; j < line.size(); j++) {
				flat_text += line[j];
				glm::vec2 char_position = char_positions[j];
				glm::vec2 char_bounding_box =
					Vulkan::Draw_Font_2D::get_char_render_bounding_box(line[j], font_id, font_size, stroke_size);
				glm::vec2 min = origin + char_position;
				glm::vec2 max = min + char_bounding_box;
				render_char_rects.push_back({min, max});
				rect_to_line_index.push_back(i);
			}
			flat_text += U'\n';
			render_char_rects.push_back(
				{{::Const::MAX_UINT32, ::Const::MAX_UINT32}, {-::Const::MAX_UINT32, -::Const::MAX_UINT32}}
			);
			rect_to_line_index.push_back(-1);
		}
		int j = 0;
		u32_to_rect_index.resize(u32_text.size(), -1);
		rect_to_u32_index.resize(render_char_rects.size(), -1);
		for (int i = 0; i < u32_text.size(); i++) {
			const auto& my_char = u32_text[i];
			if (my_char == U'\n') {
				continue;
			}
			while (j < flat_text.size()) {
				const auto& other_char = flat_text[j];
				if (my_char == other_char) {
					break;
				}
				j++;
			}
			if (j < flat_text.size()) {
				u32_to_rect_index[i] = j;
				rect_to_u32_index[j] = i;
				j++;
			}
		}
	}

	void Label_Renderer_Information::copy_attribute_to_inner_label_transform(
		const World_Transform_2D& world_transform, const glm::vec2& text_size
	) {
		inner_label_transform.color = world_transform.color;
		inner_label_transform.opacity = world_transform.opacity;
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
		if (fit_mode != LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_AUTO) {
			auto text_helper = Vulkan::Draw_Font_2D::gen_text_info_from(
				font_id, render_text, (int)horizontal_align, font_size, stroke_size, inner_label_transform.anchor
			);
			fit_transform(text_size, text_helper.get_size(), padding);
			inner_label_transform.position += -world_transform.anchor * text_size;
		} else {
			inner_label_transform.position = {0.f, 0.f};
		}
	}

	int Label_Renderer_Information::find_u32_index_by_rect_index(int rect_index) const {
		if (rect_index < 0 || rect_index >= rect_to_u32_index.size()) {
			return -1;
		}
		return rect_to_u32_index[rect_index];
	}

	int Label_Renderer_Information::find_utf8_index_by_u32_index(int u32_index) const {
		if (u32_index < 0 || u32_index >= u32_to_utf8_bytes_start.size()) {
			return -1;
		}
		return u32_to_utf8_bytes_start[u32_index];
	}

	int Label_Renderer_Information::find_u32_index_by_utf8_index(int utf8_index) const {
		if (utf8_index < 0 || utf8_index >= utf8_to_u32_index.size()) {
			return -1;
		}
		return utf8_to_u32_index[utf8_index];
	}

	int Label_Renderer_Information::find_rect_index_by_u32_index(int u32_index) const {
		if (u32_index < 0 || u32_index >= u32_to_rect_index.size()) {
			return -1;
		}
		return u32_to_rect_index[u32_index];
	}

	int Label_Renderer_Information::find_rect_index_by_char_index(int char_index) const {
		int u32_char_index = find_u32_index_by_utf8_index(char_index);
		if (u32_char_index < 0) {
			return -1;
		}
		return find_rect_index_by_u32_index(u32_char_index);
	}

	int Label_Renderer_Information::find_line_index_by_rect_index(int rect_index) const {
		if (rect_index < 0 || rect_index >= rect_to_line_index.size()) {
			return -1;
		}
		return rect_to_line_index[rect_index];
	}

	std::vector<std::pair<glm::vec2, glm::vec2>> Label_Renderer_Information::find_rects_char_by_range(
		int start_char_index, int end_char_index
	) const {
		std::vector<std::pair<glm::vec2, glm::vec2>> rects;
		start_char_index = std::max(0, start_char_index);
		end_char_index = std::min(end_char_index, (int)utf8_to_u32_index.size() - 1);
		end_char_index = std::max(start_char_index, end_char_index);
		int start_rect_index = find_rect_index_by_char_index(start_char_index);
		int end_rect_index = find_rect_index_by_char_index(end_char_index);
		int current_line_index = -1;
		int i = start_rect_index;
		while (i <= end_rect_index) {
			if (current_line_index < 0) {
				current_line_index = find_line_index_by_rect_index(i);
				if (current_line_index < 0) {
					i++;
					continue;
				}
			}
			int j = i + 1;
			while (j <= end_rect_index) {
				int line_index = find_line_index_by_rect_index(j);
				if (line_index != current_line_index || line_index < 0) {
					break;
				}
				j++;
			}
			int start = i;
			int end = j - 1;
			const auto& rect_min = render_char_rects[start];
			const auto& rect_max = render_char_rects[end];
			// Log::info("What is my rect info", start, end, rect_min.first, rect_max.second);
			rects.push_back({rect_min.first, rect_max.second});
			i = j;
			current_line_index = -1;
		}
		return rects;
	}

	float Label_Renderer_Information::get_base_line_vertical_offset() const {
		return Vulkan::Draw_Font_2D::get_base_line_vertical_renderer_offset(font_id, font_size, stroke_size);
	}

	float Label_Renderer_Information::get_line_height() const {
		return Vulkan::Draw_Font_2D::get_renderer_line_height(font_id, font_size, stroke_size);
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

	void Rect::clean_global_min_max() {
		global_min = glm::vec2{::Const::MAX_FLOAT};
		global_max = glm::vec2{-::Const::MAX_FLOAT};
	}

	void Rect::update_transformed_rect(const std::array<glm::vec2, 4>& i_transformed_rect) {
		transformed_rect = i_transformed_rect;
		min = glm::vec2{::Const::MAX_FLOAT};
		max = glm::vec2{-::Const::MAX_FLOAT};
		for (const auto& point : transformed_rect) {
			min.x = std::min(min.x, point.x);
			min.y = std::min(min.y, point.y);
			max.x = std::max(max.x, point.x);
			max.y = std::max(max.y, point.y);
		}
	}

	void Rect::update_global_min_max(const glm::vec2& child_global_min, const glm::vec2& child_global_max) {
		global_min.x = std::min(global_min.x, child_global_min.x);
		global_min.y = std::min(global_min.y, child_global_min.y);
		global_max.x = std::max(global_max.x, child_global_max.x);
		global_max.y = std::max(global_max.y, child_global_max.y);
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

	bool Key_Board_Information::has_lisenter() const {
		return key_input_enabled || char_input_enabled;
	}

	bool Key_Board_Information::can_handle_key_input() const {
		return key_input_enabled && key_input_caller != nullptr;
	}

	bool Key_Board_Information::can_handle_char_input() const {
		return char_input_enabled && char_input_caller != nullptr;
	}

} // namespace Engine