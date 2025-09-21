#include <graphic/vulkan_implementation/vulkan_core_data.h>

Graphic::Vulkan_Core_Data::Vulkan_Core_Data() {
    _instance = new Vulkan_Instance();
}

Graphic::Vulkan_Core_Data::~Vulkan_Core_Data() {
    delete(_instance);
}

void Graphic::Vulkan_Core_Data::init_data() {

    // init vulkan instance
    _instance->init();

}

Graphic::Vulkan_Instance* Graphic::Vulkan_Core_Data::get_instance() {
    return _instance;
}