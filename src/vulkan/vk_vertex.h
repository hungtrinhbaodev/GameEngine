#pragma once
#include <glm/glm.hpp>

namespace Vulkan {
	struct Vertex {
		glm::vec3 position = {0.f, 0.f, 0.f};
		glm::vec2 tex_coord = {0.f, 0.f};
		glm::vec3 normal = {0.f, 0.f, 0.f};
		glm::vec3 color = {0.f, 0.f, 0.f};
	};
} // namespace Vulkan