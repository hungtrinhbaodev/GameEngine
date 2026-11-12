#pragma once
#include <array>
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

#include <functional>
#include <geometry_structs.h>
#include <vulkan/vk_buffer.h>
#include <vulkan/vk_consts.h>
#include <vulkan/vk_font_system.h>
#include <vulkan/vk_image.h>
#include <vulkan/vk_model_3D_system.h>
#include <vulkan/vk_pipeline.h>
#include <vulkan/vk_static_buffer.h>
#include <vulkan/vk_texture_system.h>

namespace Vulkan {

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

	extern std::array<VkDescriptorPool, Const::MAX_FRAMES_IN_FLIGHT> descriptor_pools_2;

	extern Texture_System texture_system;

	extern uint32_t current_frame;

	extern std::vector<VkFence> draw_fences;

	extern std::vector<VkSemaphore> draw_semaphores;

	extern std::vector<VkSemaphore> render_finish_semaphores;

	extern std::vector<VkCommandBuffer> draw_command_buffers;

	extern Model_3D_System model_3D_system;

	extern Font_System font_system;

	extern bool frame_buffer_resize;

	Pipeline_Config make_default_pipeline_config();

	SSBO_Buffer& get_ssbo();

	Static_Buffer& get_static_buffer();

	void init_vulkan(GLFWwindow* window, std::function<void()> on_resize_finish);

	void draw_frame(float dt);

	void destroy_vulkan();

	void set_number_draw_call(uint32_t number_draw_call);

	void set_total_instance_draw(uint32_t number_instance_draw);

	void set_clear_screen_color(const glm::vec3& color, float opacity = 1.f);
} // namespace Vulkan