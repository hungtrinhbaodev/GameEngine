#pragma once
#include <array>
#include <geometry_structs.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace Vulkan {

	struct Draw_2D_Attribute {
		uint32_t draw_index = 0;
		bool is_visible = false;
	};

	struct Rectangle_Attributes {
		float width = 0.f;
		float height = 0.f;
		glm::vec2 position = {0.f, 0.f};
		glm::vec2 anchor = {0.f, 0.f};
		glm::vec3 color = {1.f, 1.f, 1.f};
		float rotation = 0;
	};

	struct Texture_2D_Attributes {
		std::string path = "";
		glm::vec2 position{0.f, 0.f};
		glm::vec2 scale = {1.f, 1.f};
		glm::vec2 anchor = {0.f, 0.f};
		Geometry::Texture_Rect_2D texture_rect{0.f, 0.f, 1.f, 1.f};
		float rotation = 0;
	};

	struct Triangle_Attribultes {
		/**
		 * @Note: 3 point of triangle to draw.
		 */
		glm::vec2 first{0.f, 0.f};
		glm::vec2 second{0.f, 0.f};
		glm::vec2 third{0.f, 0.f};
		glm::vec3 color{1.f, 1.f, 1.f};
	};
} // namespace Vulkan