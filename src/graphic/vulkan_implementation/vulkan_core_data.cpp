#include <graphic/vulkan_implementation/vulkan_core_data.h>

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
            _descriptors[draw_ID]->get_descriptor_set_layout()
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

    // init vulkan queues from device
    _vk_queues->init_queues(_vk_physical_device->get(), _vk_surface->get(), _vk_device->get());

    // init vulkan swapchain
    _vk_swapchain->init(_vk_physical_device->get(), _vk_surface->get(), _vk_device->get(), _window->get_window());

    // init vulkan render pass
    _vk_render_pass->init(_vk_device->get(), _vk_swapchain->get_format());

    // init frame buffers
    _vk_frame_buffers->init(
        _vk_device->get(), 
        _vk_render_pass->get(), 
        _vk_swapchain->get_imageviews(), 
        _vk_swapchain->get_extent()
    );

    // init vulkan uniform buffers
    _init_uniform_buffers();

    // init descriptor set and 
    // pipeline by draw ID
    _init_objects_draw_stage();
}

void Graphic::Vulkan_Core_Data::clear_data() {

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

    // destroy vulkan logical device
    _vk_device->destroy();

    // destroy vulkan surface KHR
    _vk_surface->destroy(_vk_instance->get());
    
    // destroy vulkan instance
    _vk_instance->destroy();
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
        _pipelines
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