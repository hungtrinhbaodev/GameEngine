#include <glm/gtc/constants.hpp>
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
		glm::vec2 frist_coord = {texture_rect.ratio_x, texture_rect.ratio_y};
		return {
			{{texture_rect.ratio_x, texture_rect.ratio_y},
			 {texture_rect.ratio_x, texture_rect.ratio_y + texture_rect.ratio_height},
			 {texture_rect.ratio_x + texture_rect.ratio_width, texture_rect.ratio_y + texture_rect.ratio_height},
			 {texture_rect.ratio_x + texture_rect.ratio_width, texture_rect.ratio_y}},
		};
	}

	float get_quat_rotation_angle(const glm::quat& start_rotation, const glm::quat& end_rotation) {
		float cos_half_angle = glm::abs(glm::dot(start_rotation, end_rotation));
		return 2.f * std::acos(glm::clamp(cos_half_angle, -1.f, 1.f));
	}

	float normalize_factors(float* factors, int number_factor) {
		float total_factor = 0.f;
		for (int i = 0; i < number_factor; i++) {
			total_factor += factors[i];
		}
		for (int i = 0; i < number_factor; i++) {
			if (total_factor > 0)
				factors[i] /= total_factor;
		}
		return total_factor;
	}

	float to_degree(float raidan) {
		return raidan * 180.f / glm::pi<float>();
	}

	float to_radians(float angle) {
		return angle * glm::pi<float>() / 180.f;
	}

	glm::vec2 rotate(float angle, glm::vec2 position) {
		angle = to_radians(angle);
		return {position.x * cos(angle) - position.y * sin(angle), position.x * sin(angle) + position.y * cos(angle)};
	}

	std::array<glm::vec2, 4> transform_rect(
		const glm::vec2& i_size, const glm::vec2& world_position, const glm::vec2& world_scale, float world_rotation,
		const glm::vec2& anchor
	) {
		glm::vec2 size = i_size * world_scale;
		glm::vec2 position = world_position;

		glm::vec2 bottom_left = position + Math::rotate(world_rotation, {-anchor.x * size.x, -anchor.y * size.y});
		glm::vec2 bottom_right = position + Math::rotate(world_rotation, {(1 - anchor.x) * size.x, -anchor.y * size.y});
		glm::vec2 top_right =
			position + Math::rotate(world_rotation, {(1 - anchor.x) * size.x, (1 - anchor.y) * size.y});
		glm::vec2 top_left = position + Math::rotate(world_rotation, {-anchor.x * size.x, (1 - anchor.y) * size.y});

		return {{bottom_left, bottom_right, top_right, top_left}};
	}

	bool right_side_edge(glm::vec2 start_point, glm::vec2 end_point, glm::vec2 check_point) {
		return ((check_point.x - start_point.x) * (end_point.y - start_point.y) -
				(check_point.y - start_point.y) * (end_point.x - start_point.x)) > 0;
	}

	bool point_in_rect(const std::array<glm::vec2, 4>& transformed_rect, const glm::vec2& point) {
		for (int i = 0; i < transformed_rect.size(); i++) {
			int current = i;
			int next = (current + 1) % transformed_rect.size();
			if (right_side_edge(transformed_rect[current], transformed_rect[next], point)) {
				return false;
			}
		}
		return true;
	}

} // namespace Math