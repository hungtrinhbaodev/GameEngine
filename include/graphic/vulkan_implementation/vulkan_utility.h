#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <string>
#include <stdexcept>

#include <graphic/vulkan_implementation/vulkan_constants.h>

namespace Graphic {

    class Vulkan_Utility {

        public:

        static void vk_check_action(VkResult result, std::string crash_message);

        static std::vector<const char*> query_instance_extensions();

        static std::vector<VkLayerProperties> query_instance_layer_propeties();

        static std::vector<const char*> query_instance_layer_enabled();

        static bool is_validation_layer_enabled();

        
    };
}