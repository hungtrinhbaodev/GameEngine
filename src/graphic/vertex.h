#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>


#include <vector>

namespace Graphic {

	struct Vertex {
		glm::vec3 position;
		glm::vec3 color;
		glm::vec2 uv;
	};


}