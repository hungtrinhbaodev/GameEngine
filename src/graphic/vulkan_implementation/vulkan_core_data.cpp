#include <graphic/vulkan_implementation/vulkan_core_data.h>

Graphic::Vulkan_Core_Data* Graphic::Vulkan_Core_Data::_instance = nullptr;

std::mutex Graphic::Vulkan_Core_Data::_lock_instance;

Graphic::Vulkan_Core_Data::Vulkan_Core_Data() {
    _vk_instance = new Vulkan_Instance();
    _vk_surface = new Vulkan_Surface_KHR();
    _vk_physical_device = new Vulkan_Physical_Device();
}

Graphic::Vulkan_Core_Data::~Vulkan_Core_Data() {
    delete(_vk_instance);
    delete(_vk_surface);
    delete(_vk_physical_device);
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
}

void Graphic::Vulkan_Core_Data::clear_data() {

    // destroy vulkan surface KHR
    _vk_surface->destroy(_vk_instance->get());
    
    // destroy vulkan instance
    _vk_instance->destroy();
}

Graphic::Vulkan_Instance* Graphic::Vulkan_Core_Data::get_instance() {
    return _vk_instance;
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