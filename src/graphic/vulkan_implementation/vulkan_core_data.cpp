#include <graphic/vulkan_implementation/vulkan_core_data.h>

Graphic::Vulkan_Core_Data* Graphic::Vulkan_Core_Data::_instance = nullptr;

Graphic::Vulkan_Core_Data::Vulkan_Core_Data() {
    _vk_instance = new Vulkan_Instance();
}

Graphic::Vulkan_Core_Data::~Vulkan_Core_Data() {
    delete(_vk_instance);
}

void Graphic::Vulkan_Core_Data::init_data() {

    // init vulkan instance
    _vk_instance->init();

}

void Graphic::Vulkan_Core_Data::clear_data() {
    
    // destroy vulkan instance
    _vk_instance->destroy();
}

Graphic::Vulkan_Instance* Graphic::Vulkan_Core_Data::get_instance() {
    return _vk_instance;
}

Graphic::Vulkan_Core_Data* Graphic::Vulkan_Core_Data::get() {

    static std::mutex lock_instance;

    lock_instance.lock();

    if (_instance == nullptr) {
        _instance = new Vulkan_Core_Data();
    }

    lock_instance.unlock();

    return _instance;
}