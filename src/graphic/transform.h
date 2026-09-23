#pragma once
#include <glm/glm.hpp>

namespace Graphic {
	struct Transform_2D {
		glm::vec2 position = {0.f, 0.f};
		glm::vec2 scale = {1.f, 1.f};
		float rotation = 0.f;
		unsigned char opacity = 255;
	};
} // namespace Graphic