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

#include <vulkan/vk_buffer.h>
#include <vulkan/vk_draw_package.h>
#include <vulkan/vk_image.h>
#include <vulkan/vk_instance_buffer.h>
#include <vulkan/vk_pipeline.h>
#include <vulkan/vk_ring_buffer.h>
#include <vulkan/vk_static_buffer.h>
#include <vulkan/vk_texture_system.h>

namespace Vulkan {

	extern std::shared_ptr<ThreadPool> _global_thread_pool;

	extern std::shared_ptr<Scheduler> _global_scheduler;

	extern GLFWwindow* _window;

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

	extern std::map<Const::DRAW_ID, Draw_Package*> draw_packages;

	extern std::map<Const::DRAW_ID, Instance_Buffer> instancing_buffers;

	extern std::map<Const::VERTEX_BUFFER_TYPE, Static_Buffer> global_vertex_buffers;

	extern Static_Buffer global_indices_buffer;

	extern std::vector<Buffer> uniform_buffers;

	extern std::vector<VkFence> draw_fences;

	extern std::vector<VkSemaphore> draw_semaphores;

	extern std::vector<VkSemaphore> render_finish_semaphores;

	extern std::vector<VkCommandBuffer> draw_command_buffers;

	extern bool frame_buffer_resize;

	extern float global_draw_2D_order;

	namespace Init {

		void _init_draw_packages();

		void _init_uniform_buffers();

		void _init_static_buffers();

		void _request_draw_fences();

		void _request_draw_semaphores();

		void _request_draw_command_buffers();

		void init_vulkan_core(
			GLFWwindow* window, std::shared_ptr<ThreadPool> global_thread_pool,
			std::shared_ptr<Scheduler> global_scheduler
		);

	} // namespace Init

	namespace Process {

		void _update_uniform_buffer();

		void _on_window_resize();

		void start_frame();

		void draw_frame();

		void end_frame();

	} // namespace Process

	namespace Destroy {

		void _destroy_static_buffers();

		void _destroy_uniform_buffers();

		void _destroy_draw_packages();

		void destroy_vulkan();

	} // namespace Destroy

	namespace API {

		void draw_triangle_2D(
			glm::vec2 first_position, glm::vec2 second_position, glm::vec2 third_position, glm::vec3 color
		);

		void draw_rectangle_2D(
			float x, float y, float width, float height, glm::vec3 color, float rotation = 0,
			glm::vec2 anchor_point = {0.f, 0.f}
		);

	} // namespace API
} // namespace Vulkan