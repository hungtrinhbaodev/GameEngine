#pragma once
#include <GLFW/glfw3.h>

namespace Vulkan {

	void init_surface(VkSurfaceKHR& surface, VkInstance instance, GLFWwindow* window);

	void destroy_surface(VkSurfaceKHR surface, VkInstance instance);

} // namespace Vulkan