#pragma once
#include <glm/glm.hpp>

namespace Vulkan {
	struct Uniform {
		glm::mat4 model = {};
		glm::mat4 view = {};
		glm::mat4 projection = {};
	};
} // namespace Vulkan