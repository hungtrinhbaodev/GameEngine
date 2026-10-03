#include <math_custom.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include <cmath>
#include <random>

namespace Math {
	std::random_device rd;

	std::mt19937 gen(rd());

	glm::mat4 make_scale(glm::vec3 scale) {
		return glm::scale(glm::mat4(1.0f), scale);
	}

	glm::mat4 make_scale(float scale_x, float scale_y, float scale_z) {
		return make_scale(glm::vec3{scale_x, scale_y, scale_z});
	}

	glm::mat4 make_translation(glm::vec3 translation) {
		return glm::translate(glm::mat4(1.0f), translation);
	}

	glm::mat4 make_translation(float dx, float dy, float dz) {
		return make_translation(glm::vec3{dx, dy, dz});
	}

	glm::mat4 make_rotation(glm::vec3 rotation) {
		return glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)) *
			   glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)) *
			   glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	}

	glm::mat4 make_rotation(float rx, float ry, float rz) {
		return make_rotation(glm::vec3{rx, ry, rz});
	}

	int random_int(int min, int max) {
		std::uniform_int_distribution<int> distrib(min, max);
		return distrib(gen);
	}

	float random_float(float min, float max) {
		std::uniform_real_distribution<float> distrib(min, max);
		return distrib(gen);
	}

	bool is_valid_triangle_with_clockwise(glm::vec2 first, glm::vec2 second, glm::vec2 third) {
		float d = (second.x - first.x) * (third.y - first.y) - (second.y - first.y) * (third.x - first.x);
		return d < 0;
	}

	std::array<glm::vec2, 4> make_tex_coord_from(const Geometry::Texture_Rect_2D& texture_rect) {
		glm::vec2 frist_coord = {texture_rect.raito_x, texture_rect.ratio_y};
		return {
			{{texture_rect.raito_x, texture_rect.ratio_y},
			 {texture_rect.raito_x, texture_rect.ratio_y + texture_rect.ratio_height},
			 {texture_rect.raito_x + texture_rect.ratio_width, texture_rect.ratio_y + texture_rect.ratio_height},
			 {texture_rect.raito_x + texture_rect.ratio_width, texture_rect.ratio_y}},
		};
	}
} // namespace Math