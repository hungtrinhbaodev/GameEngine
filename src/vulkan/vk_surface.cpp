#include <vulkan/vk_core.h>
#include <vulkan/vk_surface.h>
#include <vulkan/vk_utils.h>
#include <vulkan/vk_consts.h>
#include <log.h>

namespace Vulkan {

	namespace Init {

		void _init_surface(GLFWwindow* window) {
			// init vulkan surface
			Utils::vk_check_result(
				glfwCreateWindowSurface(instance, window, nullptr, &surface),
				"Vulkan window surface created successfully!",
				"Vulkan fail to create window surface!"
			);
		}

	}

	namespace Destroy {

		void _destroy_surface() {
			// clean surface KHR
			vkDestroySurfaceKHR(instance, surface, nullptr);
			Log::log_info("Vulkan destroy surface success!");
		}

	}
}