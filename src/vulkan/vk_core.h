#pragma once
#include <vector>

#include <ThreadPool.h>
#include <Scheduler.h>

#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#if !defined(_WIN32)
#include <vulkan/vulkan_beta.h>
#endif

#include <vulkan/vk_image.h>
#include <vulkan/vk_ring_buffer.h>
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

	namespace Init {

		void init_vulkan_core(GLFWwindow* window, std::shared_ptr<ThreadPool> global_thread_pool,
							  std::shared_ptr<Scheduler> global_scheduler);

	}

	namespace Destroy {

		void destroy_vulkan();

	}
} // namespace Vulkan