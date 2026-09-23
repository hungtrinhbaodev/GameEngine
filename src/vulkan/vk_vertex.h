#pragma once
#include <glm/glm.hpp>
#include <iostream>
#include <log.h>

namespace Vulkan {
	struct Vertex {
		glm::vec3 position = {0.f, 0.f, 0.f};
		glm::vec2 tex_coord = {0.f, 0.f};
		glm::vec3 normal = {0.f, 0.f, 0.f};
		glm::vec3 color = {0.f, 0.f, 0.f};
		friend std::ostream& operator<<(std::ostream& os, const Vertex& vertex) {
			os << "Vertex: position: " << vertex.position << ",  tex_coord: " << vertex.tex_coord
			   << ", normal: " << vertex.normal << ", color" << vertex.color << std::endl;
			return os;
		}
	};

	struct Vertex_2D {
		glm::vec2 position = {0.f, 0.f};
		glm::vec2 tex_coord = {0.f, 0.f};
		glm::vec3 color = {0.f, 0.f, 0.f};
	};
} // namespace Vulkan