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

	enum DESCRIPTION_TYPE {
		NODE_2D,
		LAYOUT_2D,
		IMAGE_2D,
		LABEL_2D,
		SCROLL_LAYOUT,
		INNER_CONTAINER,
		SCROLL_LISTENER_CONTAINER,
	};

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
		uint32_t z_order = 0;
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
		/**
		 * @Note: if user call set_cotent_size
		 * the image will switch on mode
		 * use custom size and they will
		 * retain untill end.
		 */
		bool use_custom_size = false;
	};

	struct Label_Renderer_Information {
		std::string font_path = "";
		std::string text = "";
		// what is actually drawn when use_custom_size is on: text after wrap / ellipsis. `text` stays untouched.
		std::string render_text = "";
		int font_size = 8;
		glm::vec3 stroke_color = {1.f, 1.f, 1.f};
		float stroke_size = 0.f;

		bool use_custom_size = false;
		bool clipping_enabled = false;
		LABEL_HORIZONTAL_ALIGN horizontal_align = LABEL_HORIZONTAL_ALIGN::LABEL_HORIZONTAL_ALIGN_CENTER;
		LABEL_VERTICAL_ALIGN vertical_align = LABEL_VERTICAL_ALIGN::LABEL_VERTICAL_ALIGN_CENTER;
		LABEL_RENDER_FIT_MODE fit_mode = LABEL_RENDER_FIT_MODE::CLIP;
		float horizontal_padding = 0.f;
		float vertical_padding = 0.f;

		Transform_2D inner_label_transform;
		void update_inner_text_by(const glm::vec2& text_size);
	};

	struct Clipping_Ancestor_Information {
		int number_clipping = 0;
		std::array<Handle, ::Const::MAX_DEPTH_CLIPPING> clipping_ancestors;
		Handle self_clipping = Handle::make_invalid_handle();
		void update_self_clipping(const Handle& e);
		void merge_with(
			const Clipping_Ancestor_Information& parent_clipping_ancestor, bool is_parent_clipping,
			const Handle& parent_handle
		);
		void add_clipping_ancestor(const Handle& ancestor);
		void remove_clipping_ancestor(const Handle& ancestor);
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
		std::function<void(Handle e, TOUCH_STATE touch_state, glm::vec2 touch_world_position)> caller = nullptr;
		std::function<void(Handle e, glm::vec2 touch_world_position)> click_caller = nullptr;
		std::array<glm::vec2, 4> transformed_rect{};
		uint32_t touched_order = 0;
	};

	struct Nine_Scale_Information {
		bool enabled = false;
		bool use_rect_ratio = false;
		Geometry::Texture_Rect_2D capset_rect_ratio{};
		Geometry::Rect_2D capset_rect{};
		glm::vec2 renderer_size{0.f, 0.f};
		std::array<World_Transform_2D, 9> pieces_transforms;
		std::array<Geometry::Texture_Rect_2D, 9> pieces_rects;
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
		float scroll_speed = 1.f;
		glm::vec2 start_touch{0.f, 0.f};
		bool swallow_scroll = true;
		std::function<void(Handle e, float delta_scroll)> caller = nullptr;
		std::array<glm::vec2, 4> transformed_rect{};
		uint32_t scroll_order = 0;
		TOUCH_SCROLL_STATE touch_state = TOUCH_SCROLL_STATE::IDLE;
	};

	struct Draw_Order {
		uint32_t value = 0;
	};

	struct Rect {
		std::array<glm::vec2, 4> transformed_rect;
		bool is_clipped_by(const Rect& parent_rect) const;
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
} // namespace Engine