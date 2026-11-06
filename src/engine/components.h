#pragma once
#include <array>
#include <consts.h>
#include <engine/structs.h>
#include <geometry_structs.h>
#include <glm/glm.hpp>
#include <handle.h>

namespace Engine {

	enum DESCRIPTION_TYPE {
		NODE_2D,
		LAYOUT_2D,
		IMAGE_2D,
		LABEL_2D,
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

	struct World_Transform_2D : Transform_2D {};

	struct Description {
		DESCRIPTION_TYPE value;
	};

	struct Size {
		glm::vec2 value;
	};

	struct Renderer_Handles {
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

} // namespace Engine