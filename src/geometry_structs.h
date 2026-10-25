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
		float ratio_x = 0.f;
		float ratio_y = 0.f;
		float ratio_width = 1.f;
		float ratio_height = 1.f;
		glm::vec4 to_vec4() const { return glm::vec4(ratio_x, ratio_y, ratio_width, ratio_height); }
		inline bool operator==(const Texture_Rect_2D& other) {
			return ratio_x == other.ratio_x && ratio_y == other.ratio_y && ratio_width == other.ratio_width &&
				   ratio_height == other.ratio_height;
		}
		inline bool operator!=(const Texture_Rect_2D& other) { return !(*this == other); }
	};

	struct Vertex_2D {
		glm::vec2 position{0.f, 0.f};
		inline friend std::ostream& operator<<(std::ostream& os, const Vertex_2D& vertex) {
			os << "{position: " << vertex.position << "}";
			return os;
		}
	};

	struct Vertex_3D {
		glm::vec3 position{0.f, 0.f, 0.f};
		glm::vec3 normal{0.f, 0.f, 0.f};
		glm::vec2 tex_coord{0.f, 0.f};
		glm::vec4 tangent{0.f, 0.f, 0.f, 0.f};
		glm::vec3 color{1.f, 1.f, 1.f};
		glm::u16vec4 joints{0, 0, 0, 0};
		glm::vec4 weights{1.f, 0.f, 0.f, 0.f};
		inline friend std::ostream& operator<<(std::ostream& os, const Vertex_3D& vertex) {
			os << "{Vertex: position: " << vertex.position << ",  tex_coord: " << vertex.tex_coord
			   << ", normal: " << vertex.normal << ", color: " << vertex.color << ", tangent: " << vertex.tangent
			   << "}";
			return os;
		}
	};

} // namespace Geometry