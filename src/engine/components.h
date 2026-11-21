#pragma once
#include <array>
#include <consts.h>
#include <engine/structs.h>
#include <engine/touch_handler.h>
#include <functional>
#include <geometry_structs.h>
#include <glm/glm.hpp>
#include <handle.h>
#include <iostream>

namespace Engine {

	struct Tree_Information {
		bool valid_child = false;
		uint32_t depth = ::Const::MAX_UINT32;
		Handle parent{};
		std::vector<Handle> children{};
		std::string name = "";
		int tag = -1;
	};

	struct Transform_2D {
		glm::vec2 position{0.f};
		glm::vec2 scale{1.f};
		float rotation = 0.f;
		float opacity = 1.f;
		glm::vec3 color{1.f, 1.f, 1.f};
		glm::vec2 anchor{0.f, 0.f};
		bool is_visible = true;
		int z_order = 0;
	};

	struct World_Transform_2D : Transform_2D {
		World_Transform_2D transform(const Transform_2D& local_transform) const;
	};

	struct Description {
		DESCRIPTION_TYPE value;
	};

	struct Inner_Renderer_Size {
		glm::vec2 value{0.f, 0.f};
	};

	struct Size {
		glm::vec2 value{0.f, 0.f};
	};

	struct Renderer_Handles {
		std::vector<Handle> value;
	};

	struct Nine_Scale_Renderer_Handles {
		std::vector<Handle> value;
	};

	struct Layout_Renderer_Information {
		bool enabled = false;
		glm::vec3 background_color = {1.f, 1.f, 1.f};
		float border_radius = 0.f;
		float border_width = 0.f;
		glm::vec3 border_color = {1.f, 1.f, 1.f};
	};

	struct Size_Pinning {
		bool enabled = false;
		float left = -::Const::MAX_FLOAT, right = -::Const::MAX_FLOAT, top = -::Const::MAX_FLOAT,
			  bottom = -::Const::MAX_FLOAT;
		bool is_changing(const Size_Pinning_Data& pinning) const;
		void update(const Size_Pinning_Data& pinning);
		Size_Pinning_Data get_size_pinning() const;
	};

	struct Position_Percent {
		bool enabled = false;
		glm::vec2 ratio_position{0.f, 0.f};
	};

	struct Size_Percent {
		bool enabled = false;
		glm::vec2 ratio_size{0.f, 0.f};
	};

	struct Dirty_Flag {
		uint64_t value = 0;
		void add_flag(uint64_t flag);
		void clean_flag(uint64_t flag);
		bool is_dirty_at(uint64_t flag) const;
	};

	struct Image_Renderer_Information {
		std::string image_path = "";
		Geometry::Texture_Rect_2D image_rect{0.f, 0.f, 1.f, 1.f};
		IMAGE_RENDER_FIT_MODE fit_mode;
		bool texture_changing = false;
		Transform_2D inner_image_transform;
		void update_transform_inner_image(
			const glm::vec2& size, const glm::vec2& self_scale, const World_Transform_2D& world_transform
		);
	};

	struct Label_Renderer_Information {
		std::string font_path = "";
		std::vector<std::u32string> lines;
		std::string text = "";
		std::vector<size_t> u32_to_utf8_bytes_start;
		std::vector<size_t> u32_to_utf8_bytes_length;
		std::vector<int> utf8_to_u32_index;
		std::u32string u32_text = U"";
		uint32_t font_id = ::Const::MAX_UINT32;

		// [Claude, 2026-11-11] what is actually drawn when use_custom_size is on: text after wrap / ellipsis.
		// `text` stays untouched. Filled by update_inner_text_by in components.cpp.
		std::u32string render_text = U"";
		int font_size = 8;
		glm::vec3 stroke_color = {1.f, 1.f, 1.f};
		float stroke_size = 0.f;

		bool clipping_enabled = false;
		LABEL_HORIZONTAL_ALIGN horizontal_align = LABEL_HORIZONTAL_ALIGN::LABEL_HORIZONTAL_ALIGN_CENTER;
		LABEL_VERTICAL_ALIGN vertical_align = LABEL_VERTICAL_ALIGN::LABEL_VERTICAL_ALIGN_CENTER;
		LABEL_RENDER_FIT_MODE fit_mode = LABEL_RENDER_FIT_MODE::LABEL_RENDER_FIT_AUTO;
		float horizontal_padding = 0.f;
		float vertical_padding = 0.f;

		Transform_2D inner_label_transform;
		glm::vec2 fit_size{0.f};

		int number_line = 0;
		std::vector<int> u32_to_rect_index;
		std::vector<int> rect_to_u32_index;
		std::vector<int> rect_to_line_index;
		std::vector<std::pair<glm::vec2, glm::vec2>> render_char_rects;

		glm::vec2 get_padding_size() const;
		void update_text(const std::string& text);
		void update_inner_text_by(const glm::vec2& text_size);
		void update_renderer_char_rects(const glm::vec2& anchor, const glm::vec2& text_size);
		void copy_attribute_to_inner_label_transform(
			const World_Transform_2D& world_transform, const glm::vec2& text_size
		);

		int find_u32_index_by_rect_index(int rect_index) const;
		int find_utf8_index_by_u32_index(int u32_index) const;
		int find_u32_index_by_utf8_index(int utf8_index) const;
		int find_rect_index_by_u32_index(int u32_index) const;
		int find_rect_index_by_char_index(int char_index) const;
		int find_line_index_by_rect_index(int rect_index) const;
		std::vector<std::pair<glm::vec2, glm::vec2>> find_rects_char_by_range(
			int start_char_index, int end_char_index
		) const;

		float get_base_line_vertical_offset() const;
		float get_line_height() const;
	};

	struct Clipping_Ancestor_Information {
		int number_clipping = 0;
		std::array<Handle, ::Const::MAX_DEPTH_CLIPPING> clipping_ancestors;
		Handle self_clipping = Handle::make_invalid_handle();
		void clean_up();
		void update_self_clipping(const Handle& e);
		void merge_with(
			const Clipping_Ancestor_Information& parent_clipping_ancestor, bool is_parent_clipping,
			const Handle& parent_handle
		);
		void add_clipping_ancestor(const Handle& ancestor);
		void remove_clipping_ancestor(const Handle& ancestor);
		std::tuple<int, std::array<Handle, ::Const::MAX_DEPTH_CLIPPING>> get_self_clipping_info() const;
		std::tuple<int, std::array<Handle, ::Const::MAX_DEPTH_CLIPPING>> get_clipping_ancestors_info() const;
	};

	struct Clipping_Information {
		bool enabled = false;
	};

	struct Touching_Information {
		bool enabled = false;
		bool swallow_touches = true;
		TOUCH_STATE touch_state = TOUCH_STATE::TOUCH_INIT;
		glm::vec2 touch_world_position{0.f};
		std::function<void(
			Handle e, TOUCH_STATE touch_state, glm::vec2 touch_world_position, MOUSE_BUTTON touch_type, int touch_mods
		)>
			caller = nullptr;
		std::function<void(Handle e, glm::vec2 touch_world_position, MOUSE_BUTTON touch_type, int touch_mods)>
			click_caller = nullptr;
		bool claim_touch = false;
		bool canceling_touch_enabled = false;
		MOUSE_BUTTON touch_type = MOUSE_BUTTON::MOUSE_BUTTON_LEFT;
		int touch_mods = -1;
	};

	struct Nine_Scale_Information {
		bool enabled = false;
		bool use_rect_ratio = false;
		Geometry::Texture_Rect_2D capset_rect_ratio{};
		Geometry::Rect_2D capset_rect{};
		glm::vec2 renderer_size{-::Const::MAX_FLOAT};
		std::array<World_Transform_2D, 9> pieces_transforms;
		std::array<Geometry::Texture_Rect_2D, 9> pieces_rects;
		bool is_manual_update_inner_size = false;

		void update_piece_transforms(
			const glm::vec2& renderer_size, const World_Transform_2D& world_transform, const glm::vec2& size,
			const Geometry::Texture_Rect_2D& rect
		);
	};

	struct Scroll_Information {
		bool enabled = false;
		Handle container_handle;
		Handle container_touch_handle;
		glm::vec2 size{0.f, 0.f};
		SCROLL_DIRECTION scroll_direction = SCROLL_DIRECTION::SCROLL_VERTICAL;
		float scroll_speed = 1.5f;
		glm::vec2 start_touch{0.f, 0.f};
		bool swallow_scroll = true;
		std::function<void(Handle e, float delta_scroll)> caller = nullptr;
		TOUCH_SCROLL_STATE touch_state = TOUCH_SCROLL_STATE::IDLE;
		float dragging_threshold = 7.5f;
	};

	struct Draw_Order {
		uint32_t value = 0;
	};

	struct Rect {
		std::array<glm::vec2, 4> transformed_rect;
		glm::vec2 min{::Const::MAX_FLOAT};
		glm::vec2 max{-::Const::MAX_FLOAT};
		glm::vec2 global_min{::Const::MAX_FLOAT};
		glm::vec2 global_max{-::Const::MAX_FLOAT};

		void clean_global_min_max();
		void update_transformed_rect(const std::array<glm::vec2, 4>& transformed_rect);
		void update_global_min_max(const glm::vec2& child_global_min, const glm::vec2& child_global_max);
		bool is_clipped_by(const Rect& parent_rect) const;
		CLIPPING_STATE get_cliping_state_with(const Rect& parent_rect) const;

		inline friend std::ostream& operator<<(std::ostream& os, const Rect& rect) {
			os << "{";
			for (int i = 0; i < rect.transformed_rect.size(); i++) {
				os << rect.transformed_rect[i];
				if (i < rect.transformed_rect.size() - 1) {
					os << ",";
				}
			}
			os << "}";
			return os;
		}
	};

	struct Key_Board_Information {
		bool key_input_enabled = false;
		std::function<void(const Handle& e, KEY key, KEY_ACTION action, int mods, bool& swallowed)> key_input_caller =
			nullptr;
		bool char_input_enabled = false;
		std::function<void(const Handle& e, char32_t code_point, bool& swallowed)> char_input_caller = nullptr;

		bool has_lisenter() const;
		bool can_handle_key_input() const;
		bool can_handle_char_input() const;
	};

	struct Focus_Caller_Information {
		std::function<void(const Handle& e)> enter_caller = nullptr;
		std::function<void(const Handle& e)> exit_caller = nullptr;
	};

	struct Edit_Box_Information {
		Handle scroll_layout = Handle::make_invalid_handle();
		Handle inner_label = Handle::make_invalid_handle();
		std::vector<Handle> background_ranges;
		Handle caret = Handle::make_invalid_handle();
		SIDE_FIX_MODE size_fit_mode = SIDE_FIX_MODE::SIZE_FIX_WIDTH;
		int caret_location = -1;
		EDIT_BOX_STATE edit_box_state = EDIT_BOX_STATE::EDIT_BOX_STATE_IDLE;
		std::string text_place_holder = "";
		std::string text_content = "";
	};
} // namespace Engine