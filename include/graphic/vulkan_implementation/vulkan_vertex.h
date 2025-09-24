#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <vector>

#include <graphic/common/vertex.h>
#include <graphic/vulkan_implementation/vulkan_constants.h>

namespace Graphic {

    class Vulkan_Vertex : public Vertex {

        public:

        static std::vector<VkVertexInputBindingDescription> get_vertex_input_binding_descriptions();

        static std::vector<VkVertexInputAttributeDescription> get_vertex_input_attribute_descriptions(Vulkan_Draw_ID draw_ID);

    };
}