
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <math_custom.h>
#include <utils.h>
#include <vulkan/vk_command_pool.h>
#include <vulkan/vk_consts.h>
#include <vulkan/vk_core.h>
#include <vulkan/vk_depth_image.h>
#include <vulkan/vk_descriptor.h>
#include <vulkan/vk_device.h>
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

	std::shared_ptr<ThreadPool> _global_thread_pool = nullptr;

	std::shared_ptr<Scheduler> _global_scheduler = nullptr;

	GLFWwindow* _window = nullptr;

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

	std::shared_ptr<Ring_Buffer> global_stagging_buffer = std::make_shared<Ring_Buffer>();

	Texture_System texture_system{};

	uint32_t current_frame = 0;

	std::map<Const::DRAW_ID, Pipeline> pipelines = {};

	std::map<Const::DRAW_ID, std::vector<std::vector<VkDescriptorSet>>> descriptor_sets_by_draw_id = {};

	std::map<Const::DRAW_ID, Instance_Buffer> instancing_buffers = {};

	Static_Buffer global_vertex_buffer = {};

	Static_Buffer global_indices_buffer = {};

	std::vector<Buffer> uniform_buffers = {};

	std::vector<VkFence> draw_fences = {};

	std::vector<VkSemaphore> draw_semaphores = {};

	std::vector<VkSemaphore> render_finish_semaphores = {};

	std::vector<VkCommandBuffer> draw_command_buffers = {};

	bool frame_buffer_resize = false;

	namespace Init {

		void _init_vulkan_pipelines() {
			/**
			 * Make vertex config to pipeline.
			 */
			Vertex_Input_Builder vertex_builder{};
			vertex_builder.add_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
				.add_attribute_description(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position))
				.add_attribute_description(0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, tex_coord))
				.add_attribute_description(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal))
				.add_attribute_description(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color));
			vertex_builder.add_binding_description(1, sizeof(glm::mat4), VK_VERTEX_INPUT_RATE_INSTANCE)
				.add_mat4_attribute_description(1, 0);

			/**
			 * Make pipeline config.
			 */
			Pipeline_Config pipeline_config{};
			pipeline_config.vertex_descriptions = vertex_builder.build_binding_descriptions();
			pipeline_config.attribute_descriptions = vertex_builder.build_attribute_descriptions();
			pipeline_config.render_pass = render_pass;
			pipeline_config.device = device;
			pipeline_config.swapchain_extent = swapchain_extent;

			/**
			 * Make descriptor set layout camera buffer to pipline
			 */
			Descriptor_Set_Layout_Builder uniform_descriptor_set_layout_builder{};
			uniform_descriptor_set_layout_builder.add_binding(
				0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT
			);

			/**
			 * Make pipeline draw primitive
			 */
			{
				std::vector<VkDescriptorSetLayout> descriptor_set_layouts{
					{uniform_descriptor_set_layout_builder.build()}
				};
				pipeline_config.descriptor_set_layouts = descriptor_set_layouts;
				pipeline_config.vertex_shader_path = ::Utils::get_root_path() + Const::PATH_VERT_SHADERD_DRAW_DEFAULT;
				pipeline_config.fragment_shader_path = ::Utils::get_root_path() + Const::PATH_FRAG_SHADERD_DRAW_DEFAULT;
				pipeline_config.depth_image = depth_image;
				Pipeline pipeline = {};
				pipeline.init(pipeline_config);
				pipelines[Const::DRAW_ID::DRAW_2D_MESH] = pipeline;

				/**
				 * Initialize descriptor set relative with pipeline
				 */
				std::vector<std::vector<VkDescriptorSet>> descriptor_set_by_frames;
				for (int i = 0; i < Const::MAX_FRAMES_IN_FLIGHT; i++) {
					Buffer& uniform_buffer = uniform_buffers[i];
					descriptor_set_by_frames.push_back({});
					/**
					 * Create uniform buffer descriptor set
					 */
					VkDescriptorSet uniform_buffer_descriptor_set{};
					VkDescriptorSetAllocateInfo allocate_info =
						Structs::make_descriptor_set_allocate_info(descriptor_pools[i], descriptor_set_layouts);
					vkAllocateDescriptorSets(device, &allocate_info, &uniform_buffer_descriptor_set);
					Descriptor_Set_Writer writer{};
					writer.add_buffer_write(0, &uniform_buffer.descriptor, uniform_buffer_descriptor_set);
					writer.write(device);
					descriptor_set_by_frames[i].push_back(uniform_buffer_descriptor_set);
				}
				descriptor_sets_by_draw_id[Const::DRAW_2D_MESH] = descriptor_set_by_frames;

				/**
				 * Initialize instancing buffer relative to draw id
				 */
				Instance_Buffer instancing_buffer{};
				instancing_buffer.init(
					global_stagging_buffer.get(), Const::INITIALIZE_SIZE_INSTANCING_BUFFER, sizeof(glm::mat4)
				);
				instancing_buffers[Const::DRAW_2D_MESH] = instancing_buffer;
			}
		}

		void _init_uniform_buffers() {
			size_t uniform_size = sizeof(Uniform);
			for (int i = 0; i < Const::MAX_FRAMES_IN_FLIGHT; i++) {
				Buffer uniform_buffer{};
				uniform_buffer.make_buffer(
					uniform_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
				);
				uniform_buffers.push_back(uniform_buffer);
			}
			Log::info("Create uniform buffers successfully!");
		}

		void _request_draw_fences() {
			for (int i = 0; i < Const::MAX_FRAMES_IN_FLIGHT; i++) {
				draw_fences.push_back(API::request_fence());
			}
			Log::info("Create draw fences successfully!");
		}

		void _init_semaphores() {
			for (int i = 0; i < Const::MAX_FRAMES_IN_FLIGHT; i++) {
				draw_semaphores.push_back(API::request_semaphore());
				render_finish_semaphores.push_back(API::request_semaphore());
			}
			Log::info("Create draw semaphores successfully!");
		}

		void _request_draw_command_buffers() {
			for (int i = 0; i < Const::MAX_FRAMES_IN_FLIGHT; i++) {
				draw_command_buffers.push_back(API::request_command_buffer());
			}
		}

		void _init_static_buffers() {
			global_vertex_buffer.init(
				global_stagging_buffer.get(), Const::INITIALIZE_STATIC_BUFFER_SIZE, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
			);
			global_indices_buffer.init(
				global_stagging_buffer.get(), Const::INITIALIZE_STATIC_BUFFER_SIZE, VK_BUFFER_USAGE_INDEX_BUFFER_BIT
			);
			/**
			 * @Note: test draw a first triangle!
			 * TODO: update to handle a system static data latter.
			 */
			triangle_vertices_id =
				global_vertex_buffer.upload_data(sizeof(Vertex) * TRIANGLE_VERTICES.size(), TRIANGLE_VERTICES.data());
			triangle_indices_id =
				global_indices_buffer.upload_data(sizeof(uint16_t) * TRIANGLE_INDICES.size(), TRIANGLE_INDICES.data());
			global_stagging_buffer->flush_frame();
			/**
			 * @Test instancing buffer to triangle first
			 */
			Instance_Buffer& instancing_buffer = instancing_buffers[Const::DRAW_2D_MESH];
			std::vector<glm::mat4> transforms{};
			for (int i = 0; i < 10000; i++) {
				glm::mat4 transform =
					Math::make_scale(0.5f, 0.35f) *
					Math::make_translation(Math::random_float(-1.0f, 1.0f), Math::random_float(-1.0f, 1.0f));
				uint32_t instancing_id = instancing_buffer.add_data(&transform);
				triangle_instancing.push_back(instancing_id);
				transforms.push_back(transform);
			}
		}

		void init_vulkan_core(
			GLFWwindow* window, std::shared_ptr<ThreadPool> global_thread_pool,
			std::shared_ptr<Scheduler> global_scheduler
		) {

			_window = window;
			glfwSetFramebufferSizeCallback(_window, [](GLFWwindow*, int, int) { Vulkan::frame_buffer_resize = true; });

			_global_thread_pool = global_thread_pool;

			_global_scheduler = global_scheduler;

			// Initialize Vulkan Instance
			_init_instance();

			// Initialize Vulkan Surface
			_init_surface();

			// Initialize Vulkan Physical Device
			_init_physical_device();

			// Initialize Vulkan Device
			_init_device();

			// Initialize Vulkan Queues
			_init_queues();

			// Initialize Vulkan Fence Pool
			_init_fences();

			// Initialize Vulkan semaphore to draw
			_init_semaphores();

			// Request some specific fences to draw
			_request_draw_fences();

			// Initialize Vulkan Command Pool By Threads
			_init_command_pool_threads();

			// Initialize Vulkan Swapchain
			_init_swapchain();

			// Initialize Vulkan Depth Image
			_init_depth_image();

			// Initialize Vulkan Render Pass
			_init_render_pass();

			// Initialize Vulkan Frame Buffer
			_init_frame_buffers();

			// Initialize Vulkan Descriptor Pools
			_init_descriptor_pools();

			// Initialize Vulkan Uniform buffers
			_init_uniform_buffers();

			// Initialize Vulkan Pipeline by each draw ID
			_init_vulkan_pipelines();

			// Request some command buffer to draw
			_request_draw_command_buffers();

			// Initialize global staging buffer
			global_stagging_buffer->init(Const::MAX_FRAMES_IN_FLIGHT, Const::INITIALIZE_SIZE_STAGING_BUFFER);

			// Initialize texture system to loading texture
			texture_system.init(Const::TEXTURE_BUCKET_SIZES, Const::NUMBER_LAYER_TEXTURE_PER_BUCKETS);

			// Initialize Vulkan static buffer to storage prototype like vertex data, index data,...
			_init_static_buffers();
		}
	} // namespace Init

	namespace Process {

		void _update_uniform_buffer() {
			const Buffer& uniform_buffer = uniform_buffers[current_frame];
			Uniform uniform{};
			/**
			 * @Note: from now we disable UBO to test
			 */
			// glm::mat4 projection = glm::perspective(
			// 	glm::radians(45.0f), (float)swapchain_extent.width / (float)swapchain_extent.height, 0.1f, 100.f
			// );
			// projection[1][1] *= -1.f;
			// uniform.projection = projection;
			// glm::mat4 view =
			// 	glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			// uniform.view = view;
			global_stagging_buffer->upload_data(uniform_buffer.buffer, 0, sizeof(Uniform), &uniform);
		}

		void _on_window_resize() {
			_recreate_swapchain();
			_recreate_depth_image();
			_recreate_frame_buffers();
		}

		void start_frame() {
			global_stagging_buffer->start_frame(current_frame);
			_update_uniform_buffer();
		}

		void draw_frame() {
			/**
			 * Flush data instancing update in frame
			 */
			for (auto& [draw_id, instancing_buffer] : instancing_buffers) {
				instancing_buffer.flush_data();
			}
			/**
			 * Flush stagging need to upload into local device buffer
			 */
			global_stagging_buffer->flush_frame();
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
					/**
					 * Bind indices buffer first
					 * because it use for all pipeline!
					 */
					vkCmdBindIndexBuffer(
						command_buffer, global_indices_buffer.inner_buffer.buffer, 0, VK_INDEX_TYPE_UINT16
					);
					for (const auto& [draw_id, pipeline_info] : pipelines) {
						VkPipeline pipeline = pipeline_info.pipeline;
						vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
						std::vector<VkDescriptorSet> descriptor_sets =
							descriptor_sets_by_draw_id[draw_id][current_frame];
						Instance_Buffer& instancing_buffer = instancing_buffers[draw_id];
						VkBuffer binding_buffers[] = {
							global_vertex_buffer.inner_buffer.buffer, instancing_buffer.inner_buffer.buffer
						};
						VkDeviceSize buffer_offsets[] = {0, 0};
						switch (draw_id) {
							case Const::DRAW_ID::DRAW_2D_MESH: {
								vkCmdBindVertexBuffers(command_buffer, 0, 2, binding_buffers, buffer_offsets);
								vkCmdBindDescriptorSets(
									command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_info.layout, 0,
									descriptor_sets.size(), descriptor_sets.data(), 0, nullptr
								);
								Static_Buffer_Range range_vertices_triangle =
									global_vertex_buffer.view_slot_info(triangle_vertices_id);
								Static_Buffer_Range range_indices_triangle =
									global_indices_buffer.view_slot_info(triangle_indices_id);
								uint32_t number_index = range_indices_triangle.size_as<uint16_t>();
								vkCmdDrawIndexed(
									command_buffer, number_index, instancing_buffer.number_instance,
									range_indices_triangle.offset, range_vertices_triangle.offset, 0
								);
								break;
							}
							default: {
								break;
							}
						}
					}
				}
				vkCmdEndRenderPass(command_buffer);
			}
			vkEndCommandBuffer(command_buffer);
			VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			VkSubmitInfo submit_info = Structs::make_submit_info(
				&command_buffer, 1, 1, &draw_semaphore, &wait_stage, 1, &finish_render_semaphore
			);
			API::submit(submit_info, draw_fence);
			VkPresentInfoKHR present_info =
				Structs::make_present_info(1, &finish_render_semaphore, 1, &swapchain, &image_index);
			result = API::submit_present(present_info);
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
	} // namespace Process

	namespace Destroy {

		void _destroy_static_buffers() {
			global_vertex_buffer.destroy();
			global_indices_buffer.destroy();
			Log::info("Destroy static buffers successfully!");
		}

		void _destroy_uniform_buffers() {
			for (auto& buffer : uniform_buffers) {
				buffer.destroy();
			}
			Log::info("Destroy uniform buffers successfully!");
		}

		void _destroy_pipelines() {
			for (auto& [draw_id, pipeline] : pipelines) {
				pipeline.destroy(device);
			}
			for (const auto& [draw_id, instancing_buffer] : instancing_buffers) {
				instancing_buffer.destroy();
			}
		}

		void destroy_vulkan() {
			// Wait to queue idle first before destroy anything
			vkQueueWaitIdle(graphics_queue);

			// Destroy static buffers
			_destroy_static_buffers();

			// Destroy texture system
			texture_system.destroy();

			// Destroy global staging buffer
			global_stagging_buffer->destroy();

			// Destroy all using pipeline
			_destroy_pipelines();

			// Destroy uniform buffer
			_destroy_uniform_buffers();

			// Destroy Vulkan Descriptor Pools
			_destroy_descriptor_pools();

			// Destroy Vulkan Frame Buffer
			_destroy_frame_buffers();

			// Destroy Vulkan Render Pass
			_destroy_render_pass();

			// Destroy Vulkan Depth Image
			_destroy_depth_image();

			// Destroy All Using Vulkan Semaphore
			_destroy_semaphores();

			// Destroy All Using Vulkan Fence
			_destroy_fences();

			// Destroy Vulkan Swapchain
			_destroy_swapchain();

			// Destroy All Vulkan Command Pool
			_destroy_command_pool_threads();

			// Destroy Vulkan Device
			_destroy_device();

			// Destroy Vulkan Surface
			_destroy_surface();

			// Destroy Vulkan Instance
			_destroy_instance();
		}

	} // namespace Destroy

} // namespace Vulkan