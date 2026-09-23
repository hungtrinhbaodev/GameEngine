#pragma once
#include <glm/glm.hpp>
#include <iostream>

#include <log.h>

namespace Vulkan {
	struct Uniform {
		glm::mat4 view = {1.f};
		glm::mat4 projection = {1.f};
		friend std::ostream& operator<<(std::ostream& os, const Uniform& uniform) {
			os << "View: " << uniform.view << std::endl << "Projection: " << uniform.projection << std::endl;
			return os;
		}
	};
} // namespace Vulkan