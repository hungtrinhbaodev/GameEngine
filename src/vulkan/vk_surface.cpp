#include <log.h>
#include <vulkan/vk_consts.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_surface.h>
#include <vulkan/vk_utils.h>

namespace Vulkan {

	namespace Init {

		void _init_surface() {
			// init vulkan surface
			Utils::vk_check_result(
				glfwCreateWindowSurface(instance, _window, nullptr, &surface),
				"Vulkan window surface created successfully!", "Vulkan fail to create window surface!"
			);
		}

	} // namespace Init

	namespace Destroy {

		void _destroy_surface() {
			// clean surface KHR
			vkDestroySurfaceKHR(instance, surface, nullptr);
			Log::log_info("Vulkan destroy surface success!");
		}

	} // namespace Destroy
} // namespace Vulkan