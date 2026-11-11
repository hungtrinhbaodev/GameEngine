#pragma once
#include <array>
#include <consts.h>
#include <geometry_structs.h>
#include <glm/glm.hpp>

namespace Math {

	glm::mat4 make_scale(glm::vec3 scale);

	glm::mat4 make_scale(float scale_x = 1.0f, float scale_y = 1.0f, float scale_z = 1.0f);

	glm::mat4 make_translation(glm::vec3 translation);

	glm::mat4 make_translation(float dx = 0.0f, float dy = 0.0f, float dz = 0.0f);

	glm::mat4 make_rotation(glm::vec3 rotation);

	glm::mat4 make_rotation(float rx = 0.0f, float ry = 0.0f, float rz = 0.0f);

	int random_int(int min = 1, int max = 100);

	float random_float(float min = 0.f, float max = 1.f);

	bool is_valid_triangle_with_clockwise(glm::vec2 first, glm::vec2 second, glm::vec2 third);

	std::array<glm::vec2, 4> make_tex_coord_from(const Geometry::Texture_Rect_2D& texture_rect);

	float get_quat_rotation_angle(const glm::quat& start_rotation, const glm::quat& end_rotation);

	float normalize_factors(float* factors, int number_factor);

	float to_degree(float raidan);

	float to_radians(float angle);

	glm::vec2 rotate(float angle, glm::vec2 position);

	std::array<glm::vec2, 4> transform_rect(
		const glm::vec2& size, const glm::vec2& world_position, const glm::vec2& world_scale, float world_rotation,
		const glm::vec2& anchor
	);

	bool right_side_edge(glm::vec2 start_point, glm::vec2 end_point, glm::vec2 check_point);

	bool point_in_rect(const std::array<glm::vec2, 4>& transformed_rect, const glm::vec2& point);

	Const::COLLISION_STATE get_collision_state(
		const std::array<glm::vec2, 4>& child_rect, const std::array<glm::vec2, 4>& parent_rect
	);

} // namespace Math