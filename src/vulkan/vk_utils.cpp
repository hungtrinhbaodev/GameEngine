#include <vulkan/vk_core.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {
	namespace Utils {
		glm::vec2 get_window_size() {
			int width, height;
			glfwGetWindowSize(Vulkan::_window, &width, &height);
			return glm::vec2(width, height);
		}
	} // namespace Utils
} // namespace Vulkan