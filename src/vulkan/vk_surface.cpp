#include <log.h>
#include <vulkan/vk_consts.h>
#include <vulkan/vk_surface.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

	namespace Init {

		void _init_surface(VkSurfaceKHR& surface, VkInstance instance, GLFWwindow* window) {
			// init vulkan surface
			Utils::vk_check_result(
				glfwCreateWindowSurface(instance, window, nullptr, &surface),
				"Vulkan window surface created successfully!", "Vulkan fail to create window surface!"
			);
		}

	} // namespace Init

	namespace Destroy {

		void _destroy_surface(VkSurfaceKHR surface, VkInstance instance) {
			// clean surface KHR
			vkDestroySurfaceKHR(instance, surface, nullptr);
			Log::info("Vulkan destroy surface success!");
		}

	} // namespace Destroy
} // namespace Vulkan