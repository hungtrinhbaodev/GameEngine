#pragma once
#include <glm/glm.hpp>
#include <iostream>

namespace Vulkan {
	struct Vertex {
		glm::vec3 position = {0.f, 0.f, 0.f};
		glm::vec2 tex_coord = {0.f, 0.f};
		glm::vec3 normal = {0.f, 0.f, 0.f};
		glm::vec3 color = {0.f, 0.f, 0.f};
		friend std::ostream& operator<<(std::ostream& os, const Vertex& vertex) {
			os << "Vertex: position{" << vertex.position.x << ", " << vertex.position.y << ", " << vertex.position.z
			   << "} tex_coord{" << vertex.tex_coord.x << ", " << vertex.tex_coord.y << "} normal{" << vertex.normal.x
			   << ", " << vertex.normal.y << ", " << vertex.normal.z << "} color{" << vertex.color.x << ", "
			   << vertex.color.y << ", " << vertex.color.z << "}" << std::endl;
			return os;
		}
	};
} // namespace Vulkan