#include <engine/components.h>
#include <math_custom.h>
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

	void Clipping_Ancestor_Information::add_clipping_ancestor(const Handle& ancestor) {
		if (::Const::MAX_DEPTH_CLIPPING <= 0)
			return;
		bool need_remove = false;
		for (int i = 0; i < number_clipping; i++) {
			if (clipping_ancesors[i].id() == ancestor.id()) {
				need_remove = true;
				break;
			}
		}
		if (need_remove) {
			remove_clipping_ancestor(ancestor);
		}
		if (number_clipping >= ::Const::MAX_DEPTH_CLIPPING) {
			remove_clipping_ancestor(clipping_ancesors[0]);
		}
		clipping_ancesors[number_clipping++] = ancestor;
	}

	void Clipping_Ancestor_Information::remove_clipping_ancestor(const Handle& ancestor) {
		if (::Const::MAX_DEPTH_CLIPPING <= 0)
			return;
		int remove_index = -1;
		for (int i = 0; i < ::Const::MAX_DEPTH_CLIPPING; i++) {
			if (clipping_ancesors[i].id() == ancestor.id()) {
				remove_index = i;
				break;
			}
		}
		if (remove_index < 0)
			return;
		for (int i = remove_index + 1; i < ::Const::MAX_DEPTH_CLIPPING; i++) {
			clipping_ancesors[i - 1] = clipping_ancesors[i];
		}
		number_clipping--;
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
					label_position.y = (text_size.x - inner_size.y) / 2;
					break;
				}
				case LABEL_VERTICAL_ALIGN::LABEL_VERTICAL_ALIGN_BOTTOM: {
					label_position.y = padding.x;
					break;
				}
				default: {
					label_position.y = text_size.y - padding.y - inner_size.y;
					break;
				}
			}
			inner_label_transform.position = label_position;
		};
		std::string final_text = text;
		auto text_helper = Vulkan::Draw_Font_2D::gen_text_info_from(
			font_path, final_text, (int)horizontal_align, font_size, stroke_size
		);
		float final_horizontal_padding =
			horizontal_align == LABEL_HORIZONTAL_ALIGN::LABEL_HORIZONTAL_ALIGN_CENTER ? 0.f : horizontal_padding;
		float final_vertical_padding =
			vertical_align == LABEL_VERTICAL_ALIGN::LABEL_VERTICAL_ALIGN_CENTER ? 0.f : vertical_padding;
		glm::vec2 padding = {final_horizontal_padding, final_vertical_padding};
		glm::vec2 inner_size = text_helper.get_size();
		if ((inner_size.x + padding.x) <= text_size.x && (inner_size.y + padding.y) <= text_size.y) {
			fit_transform(text_size, inner_size, padding);
			return;
		}
		final_text = final_text.replace(final_text.begin(), final_text.end(), "\n", " ");
		std::vector<std::string> tokens;
		while (text.find(" ") != std::string::npos) {
			auto enter_char_index = text.find("\n");
			tokens.push_back(text.substr(0, enter_char_index));
			text.erase(0, enter_char_index + 1);
		}
		if (text != "") {
			tokens.push_back(text);
		}
		int current_line = 0;
		float total_height = 0.f;
		float max_height = 0.f;
		glm::vec2 remain_size = text_size - padding;
		std::vector<std::string> lines;
		for (int i = 0; i < tokens.size(); i++) {
			const auto& token = tokens[i];
			glm::vec2 size_token = Vulkan::Draw_Font_2D::get_text_size(token, font_path, font_size, stroke_size);
			if (total_height >= remain_size.y && fit_mode == LABEL_RENDER_FIT_MODE::ELLIPSIS) {
				auto& last_line = lines.back();
				last_line = last_line.substr(0, std::min((int)last_line.size() - 3, 0)) + "...";
				break;
			}
			if (lines.size() <= current_line) {
				lines.push_back("");
			}
			auto& line = lines[current_line];
			if (line.size() < 1) {
				line += token;

				max_height = std::max(max_height, size_token.y);
				total_height += max_height;
			} else {
				glm::vec2 size_line =
					Vulkan::Draw_Font_2D::get_text_size(line + " " + token, font_path, font_size, stroke_size);
				if (size_line.x <= remain_size.x) {
					line += " " + token;
					max_height = std::max(max_height, size_line.y);
				} else {
					max_height = 0.f;
					current_line++;
					i--;
				}
			}
		}
		text = "";
		for (int i = 0; i < lines.size(); i++) {
			text += lines[i];
			if (i < lines.size() - 1) {
				text += "\n";
			}
		}
		text_helper = Vulkan::Draw_Font_2D::gen_text_info_from(
			font_path, final_text, (int)horizontal_align, font_size, stroke_size
		);
		fit_transform(text_size, text_helper.get_size(), padding);
		switch (fit_mode) {
			case LABEL_RENDER_FIT_MODE::CLIP: {
				clipping_enabled = true;
				break;
			}
			default: {
				clipping_enabled = false;
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

} // namespace Engine