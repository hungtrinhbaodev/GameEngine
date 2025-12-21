#pragma once
#include <string>
#define GLFW_INCLUDE_VULKAN
#if !defined(_WIN32_)
#include <vulkan/vulkan_beta.h>
#endif

namespace Vulkan {

	namespace Const {

#ifdef _DEBUG
		const inline bool IS_ENABLE_VALIDATION_LAYERS = true;
#elif __APPLE__
		const inline bool IS_ENABLE_VALIDATION_LAYERS = true;
#else
		const inline bool IS_ENABLE_VALIDATION_LAYERS = false;
#endif // _DEBUG

		const inline int MAX_FRAMES_IN_FLIGHT = 2;

		const inline char* VALIDATION_LAYER_NAME = "VK_LAYER_KHRONOS_validation";

		const inline std::vector<const char*> REQUIRED_PHYSICAL_DEVICE_EXTENSIONS = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if !defined(_WIN32)
			VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#endif
		};

		const inline std::string VULKAN_FENCES_SCHEDULER_TASK_NAME = "VULKAN_FENCE_SCHEDULER_TASK";

	}

}