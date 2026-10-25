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
#else
		const inline bool IS_ENABLE_VALIDATION_LAYERS = false;
#endif // _DEBUG

#if !defined(_WIN32)
		const inline uint32_t VULKAN_API_VERSION = VK_API_VERSION_1_1;
#else
		const inline uint32_t VULKAN_API_VERSION = VK_API_VERSION_1_0;
#endif

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

		const inline bool ENABLED_TEXTURE_BUCKETS = true;

		const inline std::vector<uint32_t> TEXTURE_BUCKET_SIZES{{16, 32, 64, 128, 256, 512, 1024, 2048}};

		const inline std::vector<uint32_t> NUMBER_LAYER_TEXTURE_PER_BUCKETS{{128, 128, 64, 64, 64, 16, 8, 2}};

		const inline std::vector<uint32_t> FONT_TEXTURE_BUCKET_SIZE{512, 1024, 2048};

		const inline std::vector<uint32_t> NUMBER_LAYER_FONT_TEXTURE_PER_BUCKETS{8, 4, 2};

		const inline bool ENEABLED_IMAGE_MIPMAP = true;

		enum DRAW_ID {
			DRAW_2D_MESH,					// draw primitive with raw color that user input.
			DRAW_2D_RECTANGLE_WITH_TEXTURE, // draw a rectange with texture (ui image, sprite object, ...).
			DRAW_MODEL_3D,
			DRAW_RECTANGLE_2D,
			DRAW_TRIANGLE_2D,
			DRAW_TEXTURE_2D,
			DRAW_FONT_2D,
			UNDEFINED
		};

		enum VERTEX_BUFFER_TYPE {
			NONE,
			VERTEX_2D,
			VERTEX_3D,
		};

		enum DESCRITPOR_LAYOUT {
			IMAGE_SAMPLER,
			BUCKET_IMAGE_SAMPLER,
			STORAGE_BUFFER,
			UNIFORM_BUFFER,
			EMPTY
		};

		const inline std::string SCOPE_CALCULATE_ANIMATIONS = "CALCULATE ANIMATIONS";

		const inline std::string SCOPE_UPLOAD_SKIN_DATA_ANIMATIONS = "UPLOAD SKIN DATA ANIMATIONS";

		const inline std::string SCOPE_SSBO_COPY_DATA_TO_STAGINGS = "SSBO COPY DATA TO STAGINGS";

		const inline std::string SCOPE_SSBO_COPY_DATA_TO_STORES = "SSBO COPY DATA TO STORES";

		const inline uint32_t MAX_DESCRIPTOR_STORAGE_BUFFER_BINDING = 5;

		const inline uint32_t INITIALIZE_STATIC_BUFFER_SIZE = 3 * 1024 * 1024; // 3MB

		const inline uint32_t INITIALIZE_SIZE_STAGING_BUFFER = 3 * 1024 * 1024; // 3MB

		const inline uint32_t INITIALIZE_SIZE_INSTANCING_BUFFER = 3 * 1024 * 1024; // 3MB

		const inline std::string PATH_VERT_SHADERD_DRAW_DEFAULT = "res/shader/draw_default/vert_shader.vert.spv";

		const inline std::string PATH_FRAG_SHADERD_DRAW_DEFAULT = "res/shader/draw_default/fragment_shader.frag.spv";

		const inline std::string PATH_VERT_SHADERD_DRAW_GEOMETRY_2D =
			"res/shader/draw_geometry_2D/vert_shader.vert.spv";

		const inline std::string PATH_FRAG_SHADERD_DRAW_GEOMETRY_2D =
			"res/shader/draw_geometry_2D/fragment_shader.frag.spv";

		const inline std::string PATH_VERT_SHADERD_DRAW_TEXTURE_2D = "res/shader/draw_texture_2D/vert_shader.vert.spv";

		const inline std::string PATH_FRAG_SHADERD_DRAW_TEXTURE_2D =
			"res/shader/draw_texture_2D/fragment_shader.frag.spv";

		const inline std::string PATH_FRAG_SHADERD_DRAW_TEXTURE_2D_USING_BUCKET =
			"res/shader/draw_texture_2D/fragment_shader_using_bucket.frag.spv";

		const inline std::string PATH_VERT_SHADERD_DRAW_MODEL_3D = "res/shader/draw_model_3D/vert_shader.vert.spv";

		const inline std::string PATH_FRAG_SHADERD_DRAW_MODEL_3D = "res/shader/draw_model_3D/fragment_shader.frag.spv";

		const inline std::string PATH_FRAG_SHADERD_DRAW_MODEL_3D_USING_BUCKET =
			"res/shader/draw_model_3D/fragment_shader_using_bucket.frag.spv";

		const inline std::string PATH_VERT_SHADERD_DRAW_RECTANGLE_2D =
			"res/shader/draw_rectangle_2D/vert_shader.vert.spv";

		const inline std::string PATH_FRAG_SHADERD_DRAW_RECTANGLE_2D =
			"res/shader/draw_rectangle_2D/fragment_shader.frag.spv";

		const inline std::string PATH_VERT_SHADERD_DRAW_TRIANGLE_2D =
			"res/shader/draw_triangle_2D/vert_shader.vert.spv";

		const inline std::string PATH_FRAG_SHADERD_DRAW_TRIANGLE_2D =
			"res/shader/draw_triangle_2D/fragment_shader.frag.spv";

		const inline std::string PATH_VERT_SHADERD_DRAW_FONT_2D = "res/shader/draw_font_2D/vert_shader.vert.spv";

		const inline std::string PATH_FRAG_SHADERD_DRAW_FONT_2D = "res/shader/draw_font_2D/fragment_shader.frag.spv";

		const inline std::string PATH_FRAG_SHADERD_DRAW_FONT_2D_USING_BUCKET =
			"res/shader/draw_font_2D/fragment_shader_using_bucket.frag.spv";

		const inline std::string PATH_DEFAULT_FONT = "res/fonts/anonymous_pro_bold.ttf";

	} // namespace Const

} // namespace Vulkan