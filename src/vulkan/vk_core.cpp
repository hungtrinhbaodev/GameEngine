
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <math_custom.h>
#include <utils.h>
#include <vulkan/vk_command_pool.h>
#include <vulkan/vk_consts.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_depth_image.h>
#include <vulkan/vk_descriptor.h>
#include <vulkan/vk_device.h>
#include <vulkan/vk_draw.h>
#include <vulkan/vk_draw_geometry_2D_package.h>
#include <vulkan/vk_draw_model_3D_package.h>
#include <vulkan/vk_draw_texture_2D_package.h>
#include <vulkan/vk_fences.h>
#include <vulkan/vk_frame_buffers.h>
#include <vulkan/vk_instance.h>
#include <vulkan/vk_physical_device.h>
#include <vulkan/vk_queues.h>
#include <vulkan/vk_render_pass.h>
#include <vulkan/vk_semaphores.h>
#include <vulkan/vk_structs.h>
#include <vulkan/vk_surface.h>
#include <vulkan/vk_swapchain.h>
#include <vulkan/vk_uniform.h>
#include <vulkan/vk_vertex.h>
#include <vulkan/vk_vertex_input_builder.h>

namespace Vulkan {

	std::vector<Vertex> TRIANGLE_VERTICES = {
		{{-0.7f, -0.7f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
		{{0.7f, -0.7f, 0.0f}, {0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
		{{0.7f, 0.7f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}}
	};

	std::vector<uint16_t> TRIANGLE_INDICES = {0, 1, 2};

	uint32_t triangle_vertices_id = -1;

	uint32_t triangle_indices_id = -1;

	std::vector<uint32_t> triangle_instancing{};

	GLFWwindow* window = nullptr;

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

	VkFormat swapchain_format;

	VkExtent2D swapchain_extent;

	std::vector<VkFramebuffer> frame_buffers;

	VkRenderPass render_pass = VK_NULL_HANDLE;

	std::vector<VkDescriptorPool> descriptor_pools;

	Image depth_image;

	std::shared_ptr<Ring_Buffer> global_staging_buffer = std::make_shared<Ring_Buffer>();

	Texture_System texture_system{};

	uint32_t current_frame = 0;

	std::map<Const::DRAW_ID, Pipeline> pipelines = {};

	std::map<Const::DRAW_ID, std::vector<std::vector<VkDescriptorSet>>> descriptor_sets_by_draw_id = {};

	std::map<Const::VERTEX_BUFFER_TYPE, Static_Buffer> global_vertex_buffers = {};

	std::map<Const::VERTEX_BUFFER_TYPE, Static_Buffer> global_indices_buffers = {};

	std::vector<Buffer> uniform_buffers = {};

	std::vector<VkFence> draw_fences = {};

	std::vector<VkSemaphore> draw_semaphores = {};

	std::vector<VkSemaphore> render_finish_semaphores = {};

	std::vector<VkCommandBuffer> draw_command_buffers = {};

	Model_3D_System model_3D_system = {};

	Font_System font_system{};

	SSBO_Buffer ssbo_buffer{};

	Static_Buffer_2 static_buffer{};

	bool frame_buffer_resize = false;

	float global_draw_2D_order = 0.f;

	Pipeline_Config make_default_pipeline_config() {
		Pipeline_Config pipeline_config{};
		pipeline_config.depth_image = depth_image;
		pipeline_config.device = device;
		pipeline_config.swapchain_extent = swapchain_extent;
		pipeline_config.render_pass = render_pass;
		return pipeline_config;
	}

	void request_draw_fences() {
		for (int i = 0; i < Const::MAX_FRAMES_IN_FLIGHT; i++) {
			draw_fences.push_back(request_fence(device, true));
		}
		Log::info("Create draw fences successfully!");
	}

	void request_draw_semaphores() {
		for (int i = 0; i < Const::MAX_FRAMES_IN_FLIGHT; i++) {
			draw_semaphores.push_back(request_semaphore(device));
			render_finish_semaphores.push_back(request_semaphore(device));
		}
		Log::info("Create draw semaphores successfully!");
	}

	void request_draw_command_buffers() {
		for (int i = 0; i < Const::MAX_FRAMES_IN_FLIGHT; i++) {
			draw_command_buffers.push_back(request_command_buffer(device));
		}
	}

	SSBO_Buffer& get_ssbo() {
		return ssbo_buffer;
	}

	Static_Buffer_2& get_static_buffer() {
		return static_buffer;
	}

	void init_vulkan_core(
		GLFWwindow* window, std::shared_ptr<ThreadPool> global_thread_pool, std::shared_ptr<Scheduler> global_scheduler
	) {

		Vulkan::window = window;
		glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int, int) { Vulkan::frame_buffer_resize = true; });

		// Initialize Vulkan Instance
		init_instance(instance, debug_messenger);

		// Initialize Vulkan Surface
		init_surface(surface, instance, window);

		// Initialize Vulkan Physical Device
		init_physical_device(physical_device, instance, surface);

		// Initialize Vulkan Device
		init_device(device, surface, physical_device);

		// Initialize Vulkan Queues
		init_queues(graphics_queue, present_queue, surface, physical_device, device);

		// Initialize Vulkan Fence Pool
		init_fences(device);

		// Initialize Vulkan Semaphores Pool
		init_semaphores(device);

		// Initialize Vulkan semaphore to draw
		request_draw_semaphores();

		// Request some specific fences to draw
		request_draw_fences();

		// Initialize Vulkan Command Pool By Threads
		init_command_pool_threads(surface, physical_device, device);

		// Initialize Vulkan Swapchain
		init_swapchain(
			window, swapchain, swapchain_format, swapchain_extent, swapchain_images, swapchain_image_views, surface,
			physical_device, device
		);

		// Initialize Vulkan Depth Image
		init_depth_image(depth_image, physical_device, device, swapchain_extent);

		// Initialize Vulkan Render Pass
		init_render_pass(render_pass, physical_device, device, swapchain_format);

		// Initialize Vulkan Frame Buffer
		init_frame_buffers(frame_buffers, render_pass, device, swapchain_image_views, depth_image, swapchain_extent);

		// Initialize Vulkan Descriptor Pools
		init_descriptor_pools(descriptor_pools, device);

		// Request some command buffer to draw
		request_draw_command_buffers();

		// Initialize global staging buffer
		global_staging_buffer->init(
			Const::MAX_FRAMES_IN_FLIGHT, Const::INITIALIZE_SIZE_STAGING_BUFFER, Vulkan::physical_device, Vulkan::device
		);

		static_buffer.init(
			Const::INITIALIZE_STATIC_BUFFER_SIZE, Const::INITIALIZE_STATIC_BUFFER_SIZE,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, physical_device, device
		);

		ssbo_buffer.init(Const::INITIALIZE_SIZE_STAGING_BUFFER, physical_device, device);

		// Initialize texture system to loading texture
		texture_system.init(
			Const::TEXTURE_BUCKET_SIZES, Const::NUMBER_LAYER_TEXTURE_PER_BUCKETS, device, descriptor_pools,
			physical_device
		);

		// Initialize font system to loading font
		font_system.init(device, descriptor_pools, physical_device);

		// Initialize model 3D system to loading and storage model
		model_3D_system.init(nullptr, nullptr, &texture_system, &static_buffer);

		init_draw();
	}

	void _on_window_resize() {
		recreate_swapchain(
			window, swapchain, swapchain_format, swapchain_extent, swapchain_images, swapchain_image_views, surface,
			physical_device, device
		);
		recreate_depth_image(depth_image, physical_device, device, swapchain_extent);
		recreate_frame_buffers(
			frame_buffers, render_pass, device, swapchain_image_views, depth_image, swapchain_extent
		);
	}

	void start_frame() {
		global_draw_2D_order = 1.f;
		global_staging_buffer->start_frame(current_frame);
	}

	void draw_frame() {
		global_staging_buffer->flush_frame();
		setup_draw();
		static_buffer.flush_data();

		VkFence draw_fence = draw_fences[current_frame];
		VkSemaphore draw_semaphore = draw_semaphores[current_frame];
		VkSemaphore finish_render_semaphore = render_finish_semaphores[current_frame];

		uint32_t image_index;
		vkWaitForFences(device, 1, &draw_fence, VK_TRUE, UINT64_MAX);
		VkResult result =
			vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, draw_semaphore, VK_NULL_HANDLE, &image_index);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			_on_window_resize();
			return;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		VkCommandBuffer command_buffer = draw_command_buffers[current_frame];
		vkResetFences(device, 1, &draw_fence);
		vkResetCommandBuffer(command_buffer, 0);

		VkCommandBufferBeginInfo begin_info = Structs::make_command_begin_info();
		vkBeginCommandBuffer(command_buffer, &begin_info);
		{
			/**
			 * Clear color to background and depth image before draw.
			 */
			std::vector<VkClearValue> clear_colors(2);
			clear_colors[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
			clear_colors[1].depthStencil = {1.0f, 0};
			VkRenderPassBeginInfo render_pass_info = Structs::make_render_pass_begin_info(
				render_pass, frame_buffers[image_index], swapchain_extent, clear_colors
			);
			/**
			 * Set viewport and scissor before draw.
			 */
			VkViewport viewport =
				Structs::make_draw_viewport(0, 0, swapchain_extent.width, swapchain_extent.height, 0.f, 1.f);
			VkRect2D scissor = Structs::make_scissor(swapchain_extent);
			vkCmdSetViewport(command_buffer, 0, 1, &viewport);
			vkCmdSetScissor(command_buffer, 0, 1, &scissor);

			vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
			{
				draw(command_buffer);
			}
			vkCmdEndRenderPass(command_buffer);
		}
		vkEndCommandBuffer(command_buffer);
		VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submit_info =
			Structs::make_submit_info(&command_buffer, 1, 1, &draw_semaphore, &wait_stage, 1, &finish_render_semaphore);
		submit(submit_info, device, draw_fence);
		VkPresentInfoKHR present_info =
			Structs::make_present_info(1, &finish_render_semaphore, 1, &swapchain, &image_index);
		result = submit_present(present_info, device);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || frame_buffer_resize) {
			frame_buffer_resize = false;
			_on_window_resize();
		} else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}
	}

	void end_frame() {
		current_frame = (current_frame + 1) % Const::MAX_FRAMES_IN_FLIGHT;
	}

	void destroy_vulkan() {
		// Wait to queue idle first before destroy anything
		vkQueueWaitIdle(graphics_queue);

		// Descrtroy all 2D draw component.
		destroy_draw();

		// Destroy font system
		font_system.destroy();

		// Destroy texture system
		texture_system.destroy();

		ssbo_buffer.destroy();

		static_buffer.destroy();

		// Destroy global staging buffer
		global_staging_buffer->destroy();

		// Destroy Vulkan Descriptor Pools
		destroy_descriptor_pools(descriptor_pools, device);

		// Destroy Vulkan Frame Buffer
		destroy_frame_buffers(frame_buffers, device);

		// Destroy Vulkan Render Pass
		destroy_render_pass(render_pass, device);

		// Destroy Vulkan Depth Image
		destroy_depth_image(depth_image);

		// Destroy All Using Vulkan Semaphore
		destroy_semaphores(device);

		// Destroy All Using Vulkan Fence
		destroy_fences(device);

		// Destroy Vulkan Swapchain
		destroy_swapchain(swapchain, swapchain_image_views, device);

		// Destroy All Vulkan Command Pool
		destroy_command_pool_threads(device);

		// Destroy Vulkan Device
		destroy_device(device);

		// Destroy Vulkan Surface
		destroy_surface(surface, instance);

		// Destroy Vulkan Instance
		destroy_instance(instance, debug_messenger);
	}

} // namespace Vulkan