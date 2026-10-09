#pragma once

#include <vulkan/vulkan.h>

namespace Vulkan {

	void init_instance(VkInstance& instance, VkDebugUtilsMessengerEXT& debug_messenger);

	void destroy_instance(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger);

} // namespace Vulkan
