#pragma once
#include <vector>

#include <ThreadPool.h>
#include <Scheduler.h>

#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#if !defined(_WIN32_)
#include <vulkan/vulkan_beta.h>
#endif


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

	namespace Init {

		void init_vulkan_core(
			GLFWwindow *window,
			std::shared_ptr<ThreadPool> global_thread_pool,
			std::shared_ptr<Scheduler> global_scheduler
		);

	}

	namespace Destroy {

		void destroy_vulkan();

	}
}