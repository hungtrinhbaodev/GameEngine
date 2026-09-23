#pragma once
#include <string>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>

#if !defined(_WIN32)
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

		const inline int MAX_FRAMES_IN_FLIGHT = 3;

		const inline char* VALIDATION_LAYER_NAME = "VK_LAYER_KHRONOS_validation";

		const inline std::vector<const char*> REQUIRED_PHYSICAL_DEVICE_EXTENSIONS = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if !defined(_WIN32)
			VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#endif
		};

		const inline std::string VULKAN_FENCES_SCHEDULER_TASK_NAME = "VULKAN_FENCE_SCHEDULER_TASK";

		enum TEXTURE_STORAGE_MODE {
			INVALID = -1,
			BUCKET = 0,
			INDIVIDUAL = 1
		};

		enum ASSETS_LOAD_STATE {
			UNLOAD = -1,
			LOADING = 0,
			LOADED = 1
		};

		const inline bool ENABLED_TEXTURE_BUCKETS = false;

		const inline std::vector<uint32_t> TEXTURE_BUCKET_SIZES{{16, 32, 64, 128, 256, 512}};

		const inline std::vector<uint32_t> NUMBER_LAYER_TEXTURE_PER_BUCKETS{{256, 128, 64, 64, 32, 32}};

		enum DRAW_ID {
			DRAW_2D_MESH,					// draw primitive with raw color that user input.
			DRAW_2D_RECTANGLE_WITH_TEXTURE, // draw a rectange with texture (ui image, sprite object, ...).
		};

		const inline uint32_t INITIALIZE_STATIC_BUFFER_SIZE = 3 * 1024 * 1024; // 5MB

		const inline uint32_t INITIALIZE_SIZE_STAGING_BUFFER = 3 * 1024 * 1024; // 3MB

		const inline uint32_t INITIALIZE_SIZE_INSTANCING_BUFFER = 3 * 1024 * 1024; // 3MB

		const inline std::string PATH_VERT_SHADERD_DRAW_DEFAULT = "res/shader/draw_default/vert_shader.vert.spv";

		const inline std::string PATH_FRAG_SHADERD_DRAW_DEFAULT = "res/shader/draw_default/fragment_shader.frag.spv";

	} // namespace Const

} // namespace Vulkan