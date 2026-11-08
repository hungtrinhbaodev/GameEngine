#pragma once
#include <array>
#include <consts.h>
#include <engine/structs.h>
#include <engine/touch_handler.h>
#include <functional>
#include <geometry_structs.h>
#include <glm/glm.hpp>
#include <handle.h>

namespace Engine {

	enum DESCRIPTION_TYPE {
		NODE_2D,
		LAYOUT_2D,
		IMAGE_2D,
		LABEL_2D,
		SCROLL_LAYOUT,
		INNER_CONTAINER,
	};

	struct Tree_Information {
		bool valid_child = false;
		uint32_t depth = ::Const::MAX_UINT32;
		Handle parent{};
		std::vector<Handle> children{};
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
		uint32_t draw_index = 0;
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
		float left = 0.f, right = 0.f, top = 0.f, bottom = 0.f;

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
	};

	struct Label_Renderer_Information {
		std::string font_path = "";
		std::string text = "";
		int font_size = 8;
		glm::vec3 stroke_color = {1.f, 1.f, 1.f};
		float stroke_size = 0.f;
	};

	struct Clipping_Ancestor_Information {
		int number_clipping = 0;
		std::array<Handle, ::Const::MAX_DEPTH_CLIPPING> clipping_ancesors;

		void add_clipping_ancestor(const Handle& ancestor);
		void remove_clipping_ancestor(const Handle& ancestor);
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
		glm::vec2 size{0.f, 0.f};
		SCROLL_DIRECTION scroll_direction = SCROLL_DIRECTION::SCROLL_VERTICAL;
		float scroll_speed = 1.f;
		glm::vec2 start_touch{0.f, 0.f};
		bool swallow_scroll = true;
		std::function<void(float delta_scroll)> caller = nullptr;
		std::array<glm::vec2, 4> transformed_rect{};
		uint32_t scroll_order = 0;
	};
} // namespace Engine