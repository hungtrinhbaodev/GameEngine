#pragma once
#include <map>
#include <vector>

#include <Scheduler.h>
#include <ThreadPool.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#if !defined(_WIN32)
#include <vulkan/vulkan_beta.h>
#endif

#include <geometry_structs.h>
#include <vulkan/vk_buffer.h>
#include <vulkan/vk_draw_package.h>
#include <vulkan/vk_font_system.h>
#include <vulkan/vk_image.h>
#include <vulkan/vk_instance_buffer.h>
#include <vulkan/vk_model_3D_system.h>
#include <vulkan/vk_pipeline.h>
#include <vulkan/vk_ring_buffer.h>
#include <vulkan/vk_static_buffer.h>
#include <vulkan/vk_texture_system.h>

namespace Vulkan {

	struct Core_Vulkan {

		std::shared_ptr<ThreadPool> global_thread_pool = nullptr;

		std::shared_ptr<Scheduler> global_scheduler = nullptr;

		GLFWwindow* window = nullptr;

		VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;

		VkInstance instance = VK_NULL_HANDLE;

		VkSurfaceKHR surface = VK_NULL_HANDLE;

		VkPhysicalDevice physical_device = VK_NULL_HANDLE;

		VkDevice device = VK_NULL_HANDLE;

		VkQueue graphics_queue = VK_NULL_HANDLE;

		VkQueue present_queue = VK_NULL_HANDLE;

		VkSwapchainKHR swapchain = VK_NULL_HANDLE;

		std::vector<VkImage> swapchain_images{};

		VkFormat swapchain_format{};

		VkExtent2D swapchain_extent{};

		std::vector<VkFramebuffer> frame_buffers{};

		Image depth_image{};

		VkRenderPass render_pass{};

		uint32_t current_frame = 0;

		Model_3D_System model_system{};

		Font_System font_system{};

		Texture_System texture_system{};
	};

	extern VkInstance instance;

	extern VkDebugUtilsMessengerEXT debug_messenger;

	extern VkSurfaceKHR surface;

	extern VkPhysicalDevice physical_device;

	extern VkDevice device;

	extern VkQueue graphics_queue;

	extern VkQueue present_queue;

	extern VkSwapchainKHR swapchain;

	extern std::vector<VkImage> swapchain_images;

	extern std::vector<VkImageView> swapchain_image_views;

	extern VkFormat swapchain_format;

	extern VkExtent2D swapchain_extent;

	extern std::vector<VkFramebuffer> frame_buffers;

	extern Image depth_image;

	extern VkRenderPass render_pass;

	extern std::vector<VkDescriptorPool> descriptor_pools;

	extern std::shared_ptr<Ring_Buffer> global_staging_buffer;

	extern Texture_System texture_system;

	extern uint32_t current_frame;

	extern std::map<Const::DRAW_ID, Pipeline> pipelines;

	extern std::map<Const::DRAW_ID, std::vector<std::vector<VkDescriptorSet>>> descriptor_sets_by_draw_id;

	extern std::map<Const::VERTEX_BUFFER_TYPE, Static_Buffer> global_vertex_buffers;

	extern std::map<Const::VERTEX_BUFFER_TYPE, Static_Buffer> global_indices_buffers;

	extern std::vector<Buffer> uniform_buffers;

	extern std::vector<VkFence> draw_fences;

	extern std::vector<VkSemaphore> draw_semaphores;

	extern std::vector<VkSemaphore> render_finish_semaphores;

	extern std::vector<VkCommandBuffer> draw_command_buffers;

	extern Model_3D_System model_3D_system;

	extern Font_System font_system;

	extern bool frame_buffer_resize;

	extern float global_draw_2D_order;

	Pipeline_Config make_default_pipeline_config();

	SSBO_Buffer& get_ssbo();

	Static_Buffer_2& get_static_buffer();

	void init_vulkan_core(
		GLFWwindow* window, std::shared_ptr<ThreadPool> global_thread_pool, std::shared_ptr<Scheduler> global_scheduler
	);

	void start_frame();

	void draw_frame();

	void end_frame();

	void destroy_vulkan();
} // namespace Vulkan