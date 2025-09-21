#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>

namespace Graphic {

    class Vulkan_Constants {

        public:

        static bool IS_ENABLED_VALIDATION_LAYER;

        static const char* VALIDATION_LAYER_NAME;

        static std::vector<const char*> INSTANCE_EXTENSION;

        static std::vector<const char *> REQUIRED_PHYSICAL_DEVICE_EXTENSIONS;
        
        static int MAX_FRAMES_IN_FLIGHT;
    };

}