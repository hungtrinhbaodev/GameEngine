#include <vulkan/vk_core.h>
#include <vulkan/vk_instance.h>
#include <vulkan/vk_surface.h>
#include <vulkan/vk_physical_device.h>
#include <vulkan/vk_device.h>
#include <vulkan/vk_queues.h>
#include <vulkan/vk_fences.h>
#include <vulkan/vk_command_pool.h>

namespace Vulkan {

	std::shared_ptr<ThreadPool> _global_thread_pool = nullptr;

	std::shared_ptr<Scheduler> _global_scheduler = nullptr;

	VkInstance instance = VK_NULL_HANDLE;

	VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;

	VkSurfaceKHR surface = VK_NULL_HANDLE;

	VkPhysicalDevice physical_device = VK_NULL_HANDLE;

	VkDevice device = VK_NULL_HANDLE;

	VkQueue graphics_queue = VK_NULL_HANDLE;

	VkQueue present_queue = VK_NULL_HANDLE;

	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	
	std::vector<VkImage> swapchain_images;

	std::vector<VkImageView> swapchain_image_views;

	namespace Init {

		void init_vulkan_core(
			GLFWwindow* window,
			std::shared_ptr<ThreadPool> global_thread_pool,
			std::shared_ptr<Scheduler> global_scheduler
		) {

			_global_thread_pool = global_thread_pool;

			_global_scheduler = global_scheduler;

			// Initialize Vulkan Instance
			_init_instance();

			// Initialize Vulkan Surface
			_init_surface(window);

			// Initialize Vulkan Physical Device
			_init_physical_device();

			// Initialize Vulkan Device
			_init_device();

			// Initialize Vulkan Queues
			_init_queues();
			
			// Initialize Vulkan Fence Pool
			_init_fences();

			// Initialize Vulkan Command Pool By Threads
			_init_command_pool_threads();
		}
	}

	namespace Destroy {

		void destroy_vulkan() {

			// Destroy All Vulkan Command Pool
			_destroy_command_pool_threads();

			// Destroy Vulkan Device
			_destroy_device();

			// Destroy Vulkan Surface
			_destroy_surface();

			// Destroy Vulkan Instance
			_destroy_instance();

		}

	}
	 
}