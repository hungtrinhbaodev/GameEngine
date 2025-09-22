#include <graphic/vulkan_implementation/vulkan_queue_family_indices.h>

bool Graphic::Vulkan_Queue_Family_Indices::is_complete() {
    return graphic_family.has_value() && present_family.has_value();
}