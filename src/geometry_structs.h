#pragma once
#include <glm/glm.hpp>
#include <log.h>

namespace Geometry {

	struct Rect_2D {
		float x = 0.f;
		float y = 0.f;
		float width = 0.f;
		float height = 0.f;
	};

	struct Texture_Rect_2D {
		float x = 0.f;
		float y = 0.f;
		float ratio_width = 0.f;
		float ratio_height = 0.f;
	};

	struct Vertex_3D {
		glm::vec3 position{0.f, 0.f, 0.f};
		glm::vec3 normal{0.f, 0.f, 0.f};
		glm::vec2 tex_coord{0.f, 0.f};
		glm::vec4 tangent{0.f, 0.f, 0.f, 0.f};
		glm::vec3 color{1.f, 1.f, 1.f};
		friend std::ostream& operator<<(std::ostream& os, const Vertex_3D& vertex) {
			os << "{Vertex: position: " << vertex.position << ",  tex_coord: " << vertex.tex_coord
			   << ", normal: " << vertex.normal << ", color: " << vertex.color << ", tangent: " << vertex.tangent
			   << "}";
			return os;
		}
	};

} // namespace Geometry