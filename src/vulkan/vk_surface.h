#pragma once
#include <GLFW/glfw3.h>

namespace Vulkan {

	namespace Init {
		void _init_surface(VkSurfaceKHR& surface, VkInstance instance, GLFWwindow* window);
	}

	namespace Destroy {
		void _destroy_surface(VkSurfaceKHR surface, VkInstance instance);
	}

} // namespace Vulkan