#include <graphic/vulkan_implementation/vulkan_core_data.h>
#include <utility/time_utils.h>
#include <graphic/common/graphic_constants.h>

#include <algorithm>

Graphic::Vulkan_Core_Data* Graphic::Vulkan_Core_Data::_instance = nullptr;

std::mutex Graphic::Vulkan_Core_Data::_lock_instance;

Graphic::Vulkan_Core_Data::Vulkan_Core_Data() {
    _vk_instance = new Vulkan_Instance();
    _vk_surface = new Vulkan_Surface_KHR();
    _vk_physical_device = new Vulkan_Physical_Device();
    _vk_device = new Vulkan_Device();
    _vk_queues = new Vulkan_Queues();
    _vk_swapchain = new Vulkan_Swapchain();
    _vk_render_pass = new Vulkan_Render_Pass();
    _vk_frame_buffers = new Vulkan_Frame_Buffers();
    _vk_fences = new Vulkan_Fences();
    _vk_command_pool = new Vulkan_Command_Pool();
    _vk_assets_mgr = new Vulkan_Assets_Manager();
    _vk_render_data = new Vulkan_Render_Data();
    _vk_semaphores = new Vulkan_Semaphores();
    _wp_deep_image = new Vulkan_Image();
}

Graphic::Vulkan_Core_Data::~Vulkan_Core_Data() {
    delete(_vk_instance);
    delete(_vk_surface);
    delete(_vk_physical_device);
    delete(_vk_device);
    delete(_vk_queues);
    delete(_vk_swapchain);
    delete(_vk_render_pass);
    delete(_vk_frame_buffers);
    delete(_vk_fences);
    delete(_vk_command_pool);
    delete(_vk_assets_mgr);
    delete(_vk_render_data);
    delete(_vk_semaphores);
    delete(_wp_deep_image);
}

void Graphic::Vulkan_Core_Data::_init_uniform_buffers() {
    VkDeviceSize buffer_size = sizeof(Uniform);

    for(size_t i = 0;i < Vulkan_Constants::MAX_FRAMES_IN_FLIGHT;i++){
        Vulkan_Buffer buffer{};

        buffer.make(
            buffer_size,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        buffer.map_memory();
        _vk_uniform_buffers.push_back(buffer);
    }
}

void Graphic::Vulkan_Core_Data::_init_objects_draw_stage() {
    
    // get all draw id to iterate
    std::vector<Vulkan_Draw_ID> USING_DRAW_IDS = {
        Vulkan_Draw_ID::OBJECT_DEFAULT,
        Vulkan_Draw_ID::OBJECT_WITH_TEXTURE
    };

    // iterate all id draw to make vulkan
    // descriptor and pipeline 
    for (const Vulkan_Draw_ID& draw_ID : USING_DRAW_IDS) {

        // pipeline at draw ID info
        std::string path_vert_shader, path_frag_shader;

        // make descriptor builder
        Vulkan_Descriptor_Builder descriptor_builder{};

        // add common component to descriptor builder
        descriptor_builder.add_vk_device(_vk_device->get()).
        add_uniform_buffer(&_vk_uniform_buffers);

        switch (draw_ID) {
            case Vulkan_Draw_ID::OBJECT_DEFAULT: {

                // add path for shader file in pipeline default
                path_vert_shader = Vulkan_Constants::DEFAULT_PATH_SHADER_DRAW_DEFAULT + "vert_shader.vert.spv";
                path_frag_shader = Vulkan_Constants::DEFAULT_PATH_SHADER_DRAW_DEFAULT + "fragment_shader.frag.spv";

                // Normal object just has uniform descriptor
                descriptor_builder.
                add_descriptor_layout_type(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER).
                add_descriptor_shader_flag(VK_SHADER_STAGE_VERTEX_BIT);
                break;
            }
            case Vulkan_Draw_ID::OBJECT_WITH_TEXTURE: {

                // add path for shader file in pipeline draw with texture
                path_vert_shader = Vulkan_Constants::DEFAULT_PATH_SHADER_DRAW_WITH_TEXTURE + "vert_shader.vert.spv";
                path_frag_shader = Vulkan_Constants::DEFAULT_PATH_SHADER_DRAW_WITH_TEXTURE + "fragment_shader.frag.spv";

                // Object draw with texture will be added sampler
                // to read texture
                descriptor_builder.
                add_descriptor_layout_type(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER).
                add_descriptor_shader_flag(VK_SHADER_STAGE_VERTEX_BIT).
                add_descriptor_layout_type(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER).
                add_descriptor_shader_flag(VK_SHADER_STAGE_FRAGMENT_BIT);
                break;
            }
            default: 
            {
                break;
            }
        }

        // add descriptor with draw id to map
        _descriptors[draw_ID] = descriptor_builder.build();

        // make pipeline config to create
        // it in draw ID
        Vulkan_Pipeline_Config pipeline_config{
            path_vert_shader,
            path_frag_shader,
            Vulkan_Vertex::get_vertex_input_binding_descriptions(),
            Vulkan_Vertex::get_vertex_input_attribute_descriptions(draw_ID),
            _descriptors[draw_ID]->get_descriptor_set_layouts()
        };

        // builder pipeline at draw ID
        Vulkan_Pipeline_Builder pipeline_builder{};

        // add data for buider pipeline
        pipeline_builder.
        add_pipeline_config(pipeline_config).
        add_vk_device(_vk_device->get()).
        add_vk_render_pass(_vk_render_pass->get());

        // build object pipeline at draw ID
        _pipelines[draw_ID] = pipeline_builder.build();
    }
}

void Graphic::Vulkan_Core_Data::_init_deep_image() {

    VkFormat vk_depth_format = Vulkan_Utility::find_depth_format(_vk_physical_device->get());

    Utility::Log::get()->log_info("_init_deep_image 1 my format:", vk_depth_format);

    _wp_deep_image->make(
        _vk_swapchain->get_extent().width,
        _vk_swapchain->get_extent().height,
        vk_depth_format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );

    // auto record_data = _wp_deep_image->make_transition_record_data(
    //     vk_depth_format,
    //     VK_IMAGE_LAYOUT_UNDEFINED,
    //     VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    //     nullptr
    // );

    // _vk_command_pool->record_single_commands(
    //     Vulkan_Commands_Mode::COMMANDS_MODE_SYNC,
    //     record_data.record
    // );
}

void Graphic::Vulkan_Core_Data::_destroy_deep_image() {
    _wp_deep_image->destroy(
        _vk_device->get()
    );
}

void Graphic::Vulkan_Core_Data::_clear_objects_draw_stage() {

    // clear pipeline in map draw ID
    for (const auto& [draw_ID, pipeline] : _pipelines) {
        pipeline->destroy(_vk_device->get());
        delete(pipeline);
    }

    // clear descriptors in map draw ID
    for (const auto& [draw_ID, descriptor] : _descriptors) {
        descriptor->destroy();
        delete(descriptor);
    }
}

void Graphic::Vulkan_Core_Data::init_data(Window *window) {

    // set window into data
    _window = window;
    
    // init vulkan instance
    _vk_instance->init();

    // init vulkan surface KHR
    _vk_surface->init(_vk_instance->get(), _window->get_window());

    // init vulkan physical device
    _vk_physical_device->init(_vk_instance->get(), _vk_surface->get());

    // init vulkan device
    _vk_device->init(_vk_physical_device->get(), _vk_surface->get());

    // init fences manager
    _vk_fences->init(_vk_device->get());

    // init vulkan queues from device
    _vk_queues->init_queues(
        _vk_physical_device->get(), 
        _vk_surface->get(), 
        _vk_device->get(),
        _vk_fences
    );

    // init vulkan swapchain
    _vk_swapchain->init(
        _vk_physical_device->get(), 
        _vk_surface->get(), 
        _vk_device->get(), 
        _window->get_window()
    );

    // init vulkan command pool
    _vk_command_pool->init(
        _vk_physical_device->get(),
        _vk_device->get(),
        _vk_surface->get(),
        _vk_queues
    );

    // init deep image to test
    if (Vulkan_Constants::IS_ENABLE_DEPTH_BUFFER) {
        _init_deep_image();
    }

    // init vulkan render pass
    _vk_render_pass->init(
        _vk_device->get(), 
        _vk_physical_device->get(),
        _vk_swapchain->get_format()
    );

    // init frame buffers
    _vk_frame_buffers->init(
        _vk_device->get(), 
        _vk_render_pass->get(), 
        _vk_swapchain->get_imageviews(), 
        _vk_swapchain->get_extent(),
        _wp_deep_image->get_imageview()
    );

    // init vulkan uniform buffers
    _init_uniform_buffers();

    // init descriptor set and 
    // pipeline by draw ID
    _init_objects_draw_stage();

    // init vulkan asset manager to storage resource
    _vk_assets_mgr->init_data(_descriptors[Vulkan_Draw_ID::OBJECT_WITH_TEXTURE]);

    // init vulkan render data
    _vk_render_data->init(
        _vk_assets_mgr
    );

    // init semaphores
    _vk_semaphores->init(
        _vk_device->get()
    );

    // init draw synchonization objects
    // and draw command buffers
    for (int i = 0;i < Vulkan_Constants::MAX_FRAMES_IN_FLIGHT;i++) {
        _vk_draw_fences.push_back(_vk_fences->request_item());
        _vk_draw_semaphores.push_back(_vk_semaphores->request_item());
        _vk_render_finish_semaphores.push_back(_vk_semaphores->request_item());
        _vk_draw_command_buffers.push_back(_vk_command_pool->request_draw_command_buffer());
    }
}

void Graphic::Vulkan_Core_Data::update_data() {

    // update all using fence to do callback
    // when gpu finish job
    _vk_fences->update_data();
    
}

void Graphic::Vulkan_Core_Data::update_uniform_buffer() {
    Uniform uniform {};
    _vk_uniform_buffers[_current_frame].copy_data(&uniform, sizeof(uniform));
}

void Graphic::Vulkan_Core_Data::clear_data() {

    // wait to queues idle all task
    _vk_queues->wait_to_idle();

    // destroy all semaphore is using
    _vk_semaphores->destroy();

    // destroy deep image
    if (Vulkan_Constants::IS_ENABLE_DEPTH_BUFFER) {
        _destroy_deep_image();
    }

    // destroy render data
    _vk_render_data->destroy();

    // destroy all assets vulkan use
    _vk_assets_mgr->destroy_data(_vk_device->get());

    // destroy all fence is requested
    _vk_fences->destroy();

    // destroy descriptors and pipelines in map draw ID
    _clear_objects_draw_stage();

    // destroy uniform buffers
    for (auto& buffer : _vk_uniform_buffers) {
        buffer.destroy();
    }

    // destroy frame buffers
    _vk_frame_buffers->destroy(_vk_device->get());

    // destroy render pass
    _vk_render_pass->destroy(_vk_device->get());

    // destroy vulkan swapchain
    _vk_swapchain->destroy(_vk_device->get());

    // destroy vulkan command pools
    _vk_command_pool->destroy();

    // destroy vulkan logical device
    _vk_device->destroy();

    // destroy vulkan surface KHR
    _vk_surface->destroy(_vk_instance->get());
    
    // destroy vulkan instance
    _vk_instance->destroy();
}

void Graphic::Vulkan_Core_Data::on_draw_frame() {

    double start = glfwGetTime();

    auto render_models = _vk_render_data->get_render_models();

    if (render_models.size() <= 0) return;

    VkFence vk_fence = _vk_draw_fences[_current_frame];

    VkSemaphore vk_semaphore = _vk_draw_semaphores[_current_frame];

    VkSemaphore vk_finish_semaphore = _vk_render_finish_semaphores[_current_frame];

    // STEP 1: Acquire a image in swapchain

    uint32_t image_index;

    _time_get_draw_data = glfwGetTime() - start;

    vkWaitForFences(_vk_device->get(), 1, &vk_fence, VK_TRUE, UINT64_MAX);

    Utility::Time_Utils::get()->start_track(Graphic_Constants::KEY_TRACK_TIME_ACQUIRE_IMAGE);
    VkResult result = vkAcquireNextImageKHR(
        _vk_device->get(), 
        _vk_swapchain->get(), 
        UINT64_MAX,
        vk_semaphore,
        VK_NULL_HANDLE,
        &image_index
    );
    Utility::Time_Utils::get()->end_track(Graphic_Constants::KEY_TRACK_TIME_ACQUIRE_IMAGE);

    // Utility::Log::get()->log_info("on_draw_frame 1", image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {

        // recreate swapchain
        _vk_swapchain->recreate_swapchain(
            _vk_physical_device->get(),
            _vk_surface->get(),
            _vk_device->get(),
            _window->get_window()
        );

        // recreate deep image with another size
        if (Vulkan_Constants::IS_ENABLE_DEPTH_BUFFER) {
            _destroy_deep_image();
            _init_deep_image();
        }

        // recreate frame buffer
        _vk_frame_buffers->destroy(_vk_device->get());
        _vk_frame_buffers->init (
            _vk_device->get(),
            _vk_render_pass->get(),
            _vk_swapchain->get_imageviews(),
            _vk_swapchain->get_extent(),
            _wp_deep_image->get_imageview()
        );

        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // STEP 2: update uniform buffer

    update_uniform_buffer();

    // STEP 3: set up command buffer to draw

    VkCommandBuffer vk_command_buffer = _vk_draw_command_buffers[_current_frame];
    
    vkResetFences(_vk_device->get(), 1, &vk_fence);

    vkResetCommandBuffer(vk_command_buffer, 0);

    // STEP 4: draw by data

    // TODO: add function record all draw data here
    Utility::Time_Utils::get()->start_track(Graphic_Constants::KEY_TRACK_TIME_RECORD_DRAWS);
    VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        Vulkan_Utility::vk_check_action(
            vkBeginCommandBuffer(vk_command_buffer, &begin_info),
            "failed to begin recording command buffer!"
        );

        // STEP 4.1: reset draw buffer.

        VkRenderPassBeginInfo render_pass_info{};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = _vk_render_pass->get();
        render_pass_info.framebuffer = _vk_frame_buffers->get_frame_buffer(_current_frame);
        render_pass_info.renderArea.offset = {0, 0};
        render_pass_info.renderArea.extent = _vk_swapchain->get_extent();
        
        std::vector<VkClearValue> clear_colors;
        if (Vulkan_Constants::IS_ENABLE_DEPTH_BUFFER) {
            clear_colors.resize(2);
            clear_colors[1].depthStencil = {1.0f, 0};
        }
        else {
            clear_colors.resize(1);
        }
        clear_colors[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};

        render_pass_info.clearValueCount = static_cast<uint32_t>(clear_colors.size());
        render_pass_info.pClearValues = clear_colors.data();

        // STEP 4.2: record by render pass data

        // Bind render pass to draw
        vkCmdBeginRenderPass(vk_command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

            auto vertices_buffer = _vk_render_data->get_vertices_buffer();

            auto indices_bufer = _vk_render_data->get_indices_buffer();

            VkBuffer vk_vertices_buffer = vertices_buffer->request_using_buffer();

            VkBuffer vk_indices_buffer = indices_bufer->request_using_buffer();

            for (auto& [draw_ID, models_by_texture] : render_models) {
                if (_pipelines.find(draw_ID) == _pipelines.end() || _descriptors.find(draw_ID) == _descriptors.end()) {
                    throw std::runtime_error("Draw id is not supported!");
                }
                auto& pipeline = _pipelines[draw_ID];
                auto& descriptor = _descriptors[draw_ID];
                // Bind pipeline at draw to draw
                vkCmdBindPipeline(vk_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->get());

                VkViewport viewport{};
                viewport.x = 0;
                viewport.y = 0;
                viewport.width = (float) _vk_swapchain->get_extent().width;
                viewport.height = (float) _vk_swapchain->get_extent().height;
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                vkCmdSetViewport(vk_command_buffer, 0, 1, &viewport);

                VkRect2D scissor{};
                scissor.offset = {0, 0};
                scissor.extent = _vk_swapchain->get_extent();
                vkCmdSetScissor(vk_command_buffer, 0, 1, &scissor);

                for (auto& [texture_key, models] : models_by_texture) {

                    if (models.size() <= 0) {
                        throw std::runtime_error("models is not in render data!");
                    }

                    // get common model draw id
                    Vulkan_Draw_ID draw_ID = models[0]->get_draw_id();
                    std::vector<VkDescriptorSet> vk_using_descriptor_sets;
                    vk_using_descriptor_sets.push_back(
                        descriptor->get_uniform_descriptor_set(_current_frame)
                    );
                    switch(draw_ID) {
                        case Vulkan_Draw_ID::OBJECT_WITH_TEXTURE: {
                            // get common texture of group models
                            std::shared_ptr<Vulkan_Texture> vk_texture = models[0]->get_vk_texture();

                            // add sampler texture set to using set
                            VkDescriptorSet vk_sampler_set = descriptor->get_sampler_descriptor_set(_current_frame, vk_texture->get_key());
                            vk_using_descriptor_sets.push_back(
                                vk_sampler_set
                            );
                            break;
                        }
                        default: {
                            // with object default we don't
                            // need to update texture
                            break;
                        }
                    }

                    vkCmdBindDescriptorSets(
                        vk_command_buffer,
                        VK_PIPELINE_BIND_POINT_GRAPHICS, 
                        pipeline->get_layout(),
                        0, vk_using_descriptor_sets.size(),
                        vk_using_descriptor_sets.data(),
                        0,
                        nullptr
                    );

                    for (auto& model : models) {

                        std::string model_key = model->get_key();

                        std::string mesh_key = model->get_vk_mesh()->get_key();
                            
                        // get instances buffer of model
                        Graphic::Instance_Buffer* instances_buffer = _vk_render_data->get_instances_buffer(model_key);

                        VkBuffer vk_instance_buffer = instances_buffer->request_using_buffer();

                        VkBuffer vertices_buffers_binding[] = {vk_vertices_buffer, vk_instance_buffer};

                        Vulkan_Mesh_Buffer_Offset vertices_offset = vertices_buffer->get_offset(mesh_key);

                        Vulkan_Mesh_Buffer_Offset indices_offset = indices_bufer->get_offset(mesh_key);

                        VkDeviceSize offsets[] = {vertices_offset.offset, 0};

                        vkCmdBindVertexBuffers(vk_command_buffer, 0, 2, vertices_buffers_binding, offsets);

                        vkCmdBindIndexBuffer(vk_command_buffer, vk_indices_buffer, indices_offset.offset, VK_INDEX_TYPE_UINT16);
                        
                        // std::cout << "Number instance: " << instances_buffer->get_number_instance() << std::endl;
                        vkCmdDrawIndexed(
                            vk_command_buffer,
                            static_cast<uint32_t>(indices_offset.size / (int)sizeof(uint16_t)),
                            instances_buffer->get_number_instance(), 0, 0, 0
                        );

                        instances_buffer->release_using_buffer();
                    }
                }

            }
            
            vertices_buffer->release_using_buffer();

            indices_bufer->release_using_buffer();

        vkCmdEndRenderPass(vk_command_buffer);

    if (vkEndCommandBuffer(vk_command_buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
    Utility::Time_Utils::get()->end_track(Graphic_Constants::KEY_TRACK_TIME_RECORD_DRAWS);

    // Utility::Log::get()->log_info("on_draw_frame 2", image_index);

    // STEP 5: end draw command buffer
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore wait_semaphores[] = {vk_semaphore};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &vk_command_buffer;

    VkSemaphore signal_semaphores[] = {vk_finish_semaphore};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    // Utility::Log::get()->log_info("on_draw_frame 3", image_index);

    // start = glfwGetTime();

    _vk_queues->submit_custom_commands(
        submit_info,
        vk_fence
    );

    // _time_draw_data_in_gpu = glfwGetTime() - start;

    // Utility::Log::get()->log_info("on_draw_frame 4", image_index);

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swap_chains[] = {_vk_swapchain->get()};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swap_chains;
    present_info.pImageIndices = &image_index;

    // Utility::Log::get()->log_info("on_draw_frame 5", image_index);

    _vk_queues->submit_present_commands(
        present_info
    );

    // Utility::Log::get()->log_info("on_draw_frame 6", image_index);

    _current_frame = (_current_frame + 1) % Vulkan_Constants::MAX_FRAMES_IN_FLIGHT;
}

double Graphic::Vulkan_Core_Data::get_time_prepare_draw() {
    return _time_get_draw_data;
}

double Graphic::Vulkan_Core_Data::get_time_cpu_draw() {
    return _time_draw_data_in_gpu;
}

Graphic::Vulkan_Wrapper_Data Graphic::Vulkan_Core_Data::get_wrapper_data() {
    return {
        _vk_instance,
        _vk_surface,
        _vk_physical_device,
        _vk_device,
        _vk_queues,
        _vk_swapchain,
        _vk_render_pass,
        _vk_frame_buffers,
        _descriptors,
        _pipelines,
        _vk_fences,
        _vk_command_pool,
        _vk_assets_mgr,
        _vk_render_data,
        _vk_semaphores
    };
}

Graphic::Vulkan_Raw_Data Graphic::Vulkan_Core_Data::get_raw_data() {
    return {
        _vk_instance->get(),
        _vk_surface->get(),
        _vk_physical_device->get(),
        _vk_device->get(),
        _vk_queues->get_graphics_queue(),
        _vk_queues->get_present_queue(),
        _vk_swapchain->get(),
        _vk_render_pass->get(),
        _vk_frame_buffers->get()
    };
}

Graphic::Vulkan_Core_Data* Graphic::Vulkan_Core_Data::get() {

    _lock_instance.lock();

    if (_instance == nullptr) {
        _instance = new Vulkan_Core_Data();
    }

    _lock_instance.unlock();

    return _instance;
}

void Graphic::Vulkan_Core_Data::clean_up() {
    _lock_instance.lock();

    delete(_instance);
    _instance = nullptr;

    _lock_instance.unlock();
}