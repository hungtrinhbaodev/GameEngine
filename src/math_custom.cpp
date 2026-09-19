#include <math_custom.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include <cmath>
#include <random>

namespace Math {
	std::random_device rd;

	std::mt19937 gen(rd());

	glm::mat4 make_scale(float scale_x, float scale_y, float scale_z) {
		return glm::scale(glm::mat4(1.0f), glm::vec3(scale_x, scale_y, scale_z));
	}

	glm::mat4 make_translation(float dx, float dy, float dz) {
		return glm::translate(glm::mat4(1.0f), glm::vec3(dx, dy, dz));
	}

	glm::mat4 make_rotation(float rx, float ry, float rz) {
		return glm::rotate(glm::mat4(1.0f), glm::radians(rx), glm::vec3(1.0f, 0.0f, 0.0f)) *
			   glm::rotate(glm::mat4(1.0f), glm::radians(ry), glm::vec3(0.0f, 1.0f, 0.0f)) *
			   glm::rotate(glm::mat4(1.0f), glm::radians(rz), glm::vec3(0.0f, 0.0f, 1.0f));
	}

	int random_int(int min, int max) {
		std::uniform_int_distribution<int> distrib(min, max);
		return distrib(gen);
	}

	float random_float(float min, float max) {
		std::uniform_real_distribution<float> distrib(min, max);
		return distrib(gen);
	}
} // namespace Math