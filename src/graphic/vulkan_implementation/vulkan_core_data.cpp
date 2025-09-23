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
}

Graphic::Vulkan_Core_Data::~Vulkan_Core_Data() {
    delete(_vk_instance);
    delete(_vk_surface);
    delete(_vk_physical_device);
    delete(_vk_device);
    delete(_vk_queues);
    delete(_vk_swapchain);
    delete(_vk_render_pass);
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
}

void Graphic::Vulkan_Core_Data::clear_data() {

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

Graphic::Vulkan_Instance* Graphic::Vulkan_Core_Data::get_instance() {
    return _vk_instance;
}

Graphic::Vulkan_Wrapper_Data Graphic::Vulkan_Core_Data::get_wrapper_data() {
    return {
        _vk_instance,
        _vk_surface,
        _vk_physical_device,
        _vk_device,
        _vk_queues,
        _vk_swapchain,
        _vk_render_pass
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
        _vk_render_pass->get()
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