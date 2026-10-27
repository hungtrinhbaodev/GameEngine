
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <consts.h>
#include <limits>
#include <math_custom.h>
#include <utils.h>
#include <vulkan/vk_command_pool.h>
#include <vulkan/vk_consts.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_depth_image.h>
#include <vulkan/vk_descriptor.h>
#include <vulkan/vk_device.h>
#include <vulkan/vk_draw.h>
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
#include <vulkan/vk_vertex.h>
#include <vulkan/vk_vertex_input_builder.h>

namespace Vulkan {

	struct Profiling {
		uint32_t fps_frame_count = 0;
		double fps_total_duration = 0.f;
		uint32_t draw_calls = 0;
		uint32_t number_instance_draw = 0;
		Handle handl_label_fps;
		Handle handle_label_draw_call;
		Handle handle_label_number_instance;
		Handle handle_label_gpu_time;
		Handle handle_label_cpu_time;
		Handle handle_label_gpu_memory;

		/**
		 * GPU frame time: 2 timestamps per frame in flight (begin and end of the frame command buffer),
		 * read back after the frame fence so it never waits on the GPU.
		 */
		VkQueryPool timestamp_query_pool = VK_NULL_HANDLE;
		bool is_timestamp_supported = false;
		float timestamp_period = 1.f;
		uint64_t timestamp_mask = 0;
		std::array<bool, Const::MAX_FRAMES_IN_FLIGHT> is_timestamp_written{};
		double gpu_total_ms = 0.0;
		uint32_t gpu_samples = 0;

		/**
		 * CPU frame time: from start_frame until just before submit in draw_frame, minus the time blocked on
		 * the frame fence and acquire. Submit and present are left out because MoltenVK waits there for the next
		 * drawable (VSync), which is not engine work.
		 */
		double cpu_frame_start = 0.0;
		double cpu_wait_start = 0.0;
		double cpu_wait_ms = 0.0;
		double cpu_total_ms = 0.0;
		uint32_t cpu_samples = 0;

		void init() {
			Label_2D_Description description{};
			description.path = "";
			description.font_size = 9;
			description.color = {0.f, 0.f, 0.f};
			description.stroke_color = {0.92f, 0.92f, 0.92f};
			description.stroke_size = 1.2f;
			description.draw_index = ::Const::MAX_UINT32;
			handl_label_fps = create_label(description);
			handle_label_draw_call = create_label(description);
			handle_label_number_instance = create_label(description);
			handle_label_gpu_time = create_label(description);
			handle_label_cpu_time = create_label(description);
			handle_label_gpu_memory = create_label(description);
			init_gpu_timestamps();
		}

		void init_gpu_timestamps();
		void record_frame_begin(VkCommandBuffer command_buffer, uint32_t frame_index);
		void record_frame_end(VkCommandBuffer command_buffer, uint32_t frame_index);
		void read_frame_timestamps(uint32_t frame_index);
		std::string make_gpu_memory_text() const;
		void destroy();

		void start_cpu_frame() {
			cpu_frame_start = ::Utils::now();
			cpu_wait_ms = 0.0;
		}

		void start_cpu_wait() { cpu_wait_start = ::Utils::now(); }

		void end_cpu_wait() { cpu_wait_ms += (::Utils::now() - cpu_wait_start) * 1000.0; }

		void end_cpu_frame() {
			cpu_total_ms += (::Utils::now() - cpu_frame_start) * 1000.0 - cpu_wait_ms;
			cpu_samples++;
		}

		static std::string format_ms(double total_ms, uint32_t samples) {
			if (samples == 0) {
				return "n/a";
			}
			char text[32];
			snprintf(text, sizeof(text), "%.2f ms", total_ms / samples);
			return text;
		}

		void update_gpu_labels() {
			modify_label(handle_label_gpu_time)
				.set_text("GPU: " + (is_timestamp_supported ? format_ms(gpu_total_ms, gpu_samples) : "n/a"));
			modify_label(handle_label_cpu_time).set_text("CPU: " + format_ms(cpu_total_ms, cpu_samples));
			modify_label(handle_label_gpu_memory).set_text("GPU memory: " + make_gpu_memory_text());
			gpu_total_ms = 0.0;
			gpu_samples = 0;
			cpu_total_ms = 0.0;
			cpu_samples = 0;
		}

		void reload_labels() {
			glm::vec2 start_position = {7.f, 6.f};
			float pad = 1.3f;
			std::array<Handle, 6> labels = {
				handl_label_fps,	   handle_label_draw_call, handle_label_number_instance,
				handle_label_gpu_time, handle_label_cpu_time,  handle_label_gpu_memory,
			};
			for (auto& label_handle : labels) {
				auto& label = modify_label(label_handle);
				label.set_position(start_position);
				start_position.y += pad + label.get_height();
			}
		}

		void update_draw_call_labels() {
			auto& label_draw_call = modify_label(handle_label_draw_call);
			label_draw_call.set_text("Draw calls: " + std::to_string(draw_calls));
			auto& label_number_instance = modify_label(handle_label_number_instance);
			label_number_instance.set_text("Instances drawed: " + std::to_string(number_instance_draw));
			reload_labels();
		}

		void update_label_fps(uint32_t frame_count, float dt) {
			if (frame_count % 20 == 0) {
				auto& label = modify_label(handl_label_fps);
				double fps = fps_total_duration == 0 ? 0.f : double(fps_frame_count) / fps_total_duration;
				label.set_text("Frame rate: " + std::to_string(fps));
				fps_frame_count = 0;
				fps_total_duration = 0.f;
				update_gpu_labels();
			} else {
				fps_frame_count++;
				fps_total_duration += dt;
			}
			reload_labels();
		}
	};

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

	std::array<VkDescriptorPool, Const::MAX_FRAMES_IN_FLIGHT> descriptor_pools_2{};

	Image depth_image;

	Texture_System texture_system{};

	uint32_t current_frame = 0;

	std::vector<VkFence> draw_fences = {};

	std::vector<VkSemaphore> draw_semaphores = {};

	std::vector<VkSemaphore> render_finish_semaphores = {};

	std::vector<VkCommandBuffer> draw_command_buffers = {};

	Model_3D_System model_3D_system = {};

	Font_System font_system{};

	SSBO_Buffer ssbo_buffer{};

	Static_Buffer static_buffer{};

	bool frame_buffer_resize = false;

	uint32_t fps_frame_count = 0;

	double fps_total_duration = 0.f;

	Handle label_fps{};

	Profiling profiling{};

	void Profiling::init_gpu_timestamps() {
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(physical_device, &properties);
		timestamp_period = properties.limits.timestampPeriod;

		uint32_t graphic_family =
			Utils::query_suitable_queue_family_indices(physical_device, surface).graphic_family.value();
		uint32_t number_family = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &number_family, nullptr);
		std::vector<VkQueueFamilyProperties> families(number_family);
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &number_family, families.data());
		uint32_t valid_bits = families[graphic_family].timestampValidBits;
		is_timestamp_supported = valid_bits > 0;
		if (!is_timestamp_supported) {
			Log::info("GPU timestamps not supported on the graphics queue, GPU time label shows n/a.");
			return;
		}
		timestamp_mask = valid_bits >= 64 ? ~0ull : ((1ull << valid_bits) - 1);

		VkQueryPoolCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		create_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
		create_info.queryCount = 2 * Const::MAX_FRAMES_IN_FLIGHT;
		Utils::vk_check_result(
			vkCreateQueryPool(device, &create_info, nullptr, &timestamp_query_pool),
			"Vulkan create timestamp query pool successfully!", "Vulkan fail to create timestamp query pool!"
		);
	}

	void Profiling::record_frame_begin(VkCommandBuffer command_buffer, uint32_t frame_index) {
		if (!is_timestamp_supported) {
			return;
		}
		vkCmdResetQueryPool(command_buffer, timestamp_query_pool, 2 * frame_index, 2);
		vkCmdWriteTimestamp(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, timestamp_query_pool, 2 * frame_index);
	}

	void Profiling::record_frame_end(VkCommandBuffer command_buffer, uint32_t frame_index) {
		if (!is_timestamp_supported) {
			return;
		}
		vkCmdWriteTimestamp(
			command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, timestamp_query_pool, 2 * frame_index + 1
		);
		is_timestamp_written[frame_index] = true;
	}

	void Profiling::read_frame_timestamps(uint32_t frame_index) {
		if (!is_timestamp_supported || !is_timestamp_written[frame_index]) {
			return;
		}
		is_timestamp_written[frame_index] = false;
		std::array<uint64_t, 2> timestamps{};
		VkResult result = vkGetQueryPoolResults(
			device, timestamp_query_pool, 2 * frame_index, 2, sizeof(timestamps), timestamps.data(), sizeof(uint64_t),
			VK_QUERY_RESULT_64_BIT
		);
		if (result != VK_SUCCESS) {
			return;
		}
		uint64_t ticks = ((timestamps[1] & timestamp_mask) - (timestamps[0] & timestamp_mask)) & timestamp_mask;
		gpu_total_ms += (double)ticks * timestamp_period / 1000000.0;
		gpu_samples++;
	}

	std::string Profiling::make_gpu_memory_text() const {
		if (!is_memory_budget_enabled()) {
			return "n/a";
		}
		auto get_memory_properties_2 = (PFN_vkGetPhysicalDeviceMemoryProperties2)vkGetInstanceProcAddr(
			instance, "vkGetPhysicalDeviceMemoryProperties2"
		);
		if (get_memory_properties_2 == nullptr) {
			return "n/a";
		}
		VkPhysicalDeviceMemoryBudgetPropertiesEXT budget{};
		budget.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;
		VkPhysicalDeviceMemoryProperties2 properties{};
		properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
		properties.pNext = &budget;
		get_memory_properties_2(physical_device, &properties);
		// Only device local heaps: VRAM on a discrete GPU, the one unified heap on Apple Silicon.
		VkDeviceSize used = 0;
		VkDeviceSize available = 0;
		for (uint32_t i = 0; i < properties.memoryProperties.memoryHeapCount; i++) {
			if (properties.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
				used += budget.heapUsage[i];
				available += budget.heapBudget[i];
			}
		}
		const double mb = 1024.0 * 1024.0;
		char text[64];
		snprintf(text, sizeof(text), "%.1f / %.0f MB", used / mb, available / mb);
		return text;
	}

	void Profiling::destroy() {
		if (timestamp_query_pool != VK_NULL_HANDLE) {
			vkDestroyQueryPool(device, timestamp_query_pool, nullptr);
			timestamp_query_pool = VK_NULL_HANDLE;
		}
	}

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

	Static_Buffer& get_static_buffer() {
		return static_buffer;
	}

	void init_vulkan_core(GLFWwindow* window) {

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

		ssbo_buffer.init(Const::INITIALIZE_SIZE_STAGING_BUFFER, physical_device, device);

		// Initialize Vulkan Swapchain
		init_swapchain(
			window, swapchain, swapchain_format, swapchain_extent, swapchain_images, swapchain_image_views, surface,
			physical_device, device, ssbo_buffer
		);

		// Initialize Vulkan Depth Image
		init_depth_image(depth_image, physical_device, device, swapchain_extent);

		// Initialize Vulkan Render Pass
		init_render_pass(render_pass, physical_device, device, swapchain_format);

		// Initialize Vulkan Frame Buffer
		init_frame_buffers(frame_buffers, render_pass, device, swapchain_image_views, depth_image, swapchain_extent);

		// Initialize Vulkan Descriptor Pools
		init_descriptor_pools(descriptor_pools, descriptor_pools_2, device);

		// Request some command buffer to draw
		request_draw_command_buffers();

		static_buffer.init(
			Const::INITIALIZE_STATIC_BUFFER_SIZE, Const::INITIALIZE_STATIC_BUFFER_SIZE,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, draw_fences, physical_device, device
		);

		// Initialize texture system to loading texture
		texture_system.init(
			Const::TEXTURE_BUCKET_SIZES, Const::NUMBER_LAYER_TEXTURE_PER_BUCKETS, device, descriptor_pools,
			descriptor_pools_2, physical_device
		);

		// Initialize font system to loading font
		font_system.init(&ssbo_buffer, device, descriptor_pools, descriptor_pools_2, physical_device);

		// Initialize model 3D system to loading and storage model
		model_3D_system.init(&texture_system, &static_buffer, &ssbo_buffer);

		init_draw();

		profiling.init();
	}

	void on_window_resize() {
		recreate_swapchain(
			window, swapchain, swapchain_format, swapchain_extent, swapchain_images, swapchain_image_views, surface,
			physical_device, device, ssbo_buffer
		);
		recreate_depth_image(depth_image, physical_device, device, swapchain_extent);
		recreate_frame_buffers(
			frame_buffers, render_pass, device, swapchain_image_views, depth_image, swapchain_extent
		);
	}

	void start_frame(float dt) {
		profiling.start_cpu_frame();
		/**
		 * @Note: static buffer using for all frame
		 * so when start every frame we need confirm
		 * that all synchronization finish!
		 */
		static_buffer.wait_synchronize_finish(current_frame);
		ssbo_buffer.setup_frame(current_frame);
		setup_draw(dt);
		// if (frame_count % 20 == 0) {
		// 	Label_2D& label = modify_label(label_fps);
		// 	double fps = fps_total_duration == 0 ? 0.f : double(fps_frame_count) / fps_total_duration;
		// 	label.set_text("Frame rate: " + std::to_string(fps));
		// 	fps_frame_count = 0;
		// 	fps_total_duration = 0.f;
		// } else {
		// 	fps_frame_count++;
		// 	fps_total_duration += dt;
		// }
		profiling.update_label_fps(frame_count, dt);
	}

	void draw_frame(float dt) {
		VkFence draw_fence = draw_fences[current_frame];
		VkSemaphore draw_semaphore = draw_semaphores[current_frame];
		VkSemaphore finish_render_semaphore = render_finish_semaphores[current_frame];

		uint32_t image_index;
		profiling.start_cpu_wait();
		vkWaitForFences(device, 1, &draw_fence, VK_TRUE, UINT64_MAX);
		VkResult result =
			vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, draw_semaphore, VK_NULL_HANDLE, &image_index);
		profiling.end_cpu_wait();
		profiling.read_frame_timestamps(current_frame);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			on_window_resize();
			return;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		VkCommandBuffer command_buffer = draw_command_buffers[current_frame];
		vkResetFences(device, 1, &draw_fence);
		vkResetCommandBuffer(command_buffer, 0);

		VkCommandBufferBeginInfo begin_info = Structs::make_command_begin_info();
		vkBeginCommandBuffer(command_buffer, &begin_info);
		profiling.record_frame_begin(command_buffer, current_frame);
		{
			/**
			 * Setup static buffer using for all frame in flight.
			 */
			static_buffer.flush_removed_data(current_frame);
			static_buffer.flush_data(command_buffer, current_frame);
			/**
			 * Set up ssbo local device to use as big storage
			 */
			ssbo_buffer.flush_data_to_staging_buffer(current_frame);
			ssbo_buffer.flush_data_to_storage_buffer(command_buffer, current_frame);
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
			/**
			 * @Note: because instance buffer need to set up first
			 * before being in render so we sort draw and record first.
			 */
			sort_draws();
			batching_draw_groups(command_buffer, current_frame);
			vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
			{
				draw(command_buffer);
			}
			vkCmdEndRenderPass(command_buffer);
		}
		profiling.record_frame_end(command_buffer, current_frame);
		vkEndCommandBuffer(command_buffer);
		profiling.end_cpu_frame();
		VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submit_info =
			Structs::make_submit_info(&command_buffer, 1, 1, &draw_semaphore, &wait_stage, 1, &finish_render_semaphore);
		submit(submit_info, device, draw_fence);
		VkPresentInfoKHR present_info =
			Structs::make_present_info(1, &finish_render_semaphore, 1, &swapchain, &image_index);
		result = submit_present(present_info, device);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || frame_buffer_resize) {
			frame_buffer_resize = false;
			on_window_resize();
		} else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}
		current_frame = (current_frame + 1) % Const::MAX_FRAMES_IN_FLIGHT;
	}

	void destroy_vulkan() {
		// Wait to queue idle first before destroy anything
		vkQueueWaitIdle(graphics_queue);

		// Descrtroy all 2D draw component.
		destroy_draw();

		// Destroy profiling GPU timestamp query pool
		profiling.destroy();

		// Destroy font system
		font_system.destroy();

		// Destroy texture system
		texture_system.destroy();

		ssbo_buffer.destroy();

		static_buffer.destroy();

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

	void set_number_draw_call(uint32_t number_draw_call) {
		profiling.draw_calls = number_draw_call;
		profiling.update_draw_call_labels();
	}

	void set_total_instance_draw(uint32_t number_instance_draw) {
		profiling.number_instance_draw = number_instance_draw;
		profiling.update_draw_call_labels();
	}

} // namespace Vulkan