#pragma once
#include <array>
#include <glm/glm.hpp>
#include <graphic/transform.h>

namespace Graphic {
	struct Triangle {
		std::array<glm::vec2, 3> points{{{0.f, 0.f}, {0.f, 0.f}, {0.f, 0.f}}};
		Transform_2D transform{};
		glm::vec3 color{0.f, 0.f, 0.f};
	};
} // namespace Graphic