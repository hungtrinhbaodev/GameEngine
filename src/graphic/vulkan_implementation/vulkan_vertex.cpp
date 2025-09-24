#include <graphic/vulkan_implementation/vulkan_vertex.h>

std::vector<VkVertexInputBindingDescription> Graphic::Vulkan_Vertex::get_vertex_input_binding_descriptions() {
    
    std::vector<VkVertexInputBindingDescription> binding_descriptions(2);

    // binding for info a vertex per read
    binding_descriptions[0].binding = 0;
    binding_descriptions[0].stride = sizeof(Vulkan_Vertex);
    binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // binding for instance per read
    binding_descriptions[1].binding = 1;
    // TODO: change to formal transform class later
    // now using glm::mat4 at model matrix
    binding_descriptions[1].stride = sizeof(glm::mat4);
    binding_descriptions[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    return binding_descriptions;
}

std::vector<VkVertexInputAttributeDescription> Graphic::Vulkan_Vertex::get_vertex_input_attribute_descriptions(Vulkan_Draw_ID draw_ID) {

    std::vector<VkVertexInputAttributeDescription> attribute_descriptions{};

    uint32_t next_location = -1;

    attribute_descriptions.push_back({
        ++next_location, 0,
        VK_FORMAT_R32G32B32_SFLOAT,
        offsetof(Vulkan_Vertex, position)
    });

    attribute_descriptions.push_back({
        ++next_location, 0,
        VK_FORMAT_R32G32B32_SFLOAT,
        offsetof(Vulkan_Vertex, color)
    });

    attribute_descriptions.push_back({
        ++next_location, 0,
        VK_FORMAT_R32G32B32_SFLOAT,
        offsetof(Vulkan_Vertex, normal)
    });

    switch (draw_ID)
    {
        case Vulkan_Draw_ID::OBJECT_WITH_TEXTURE: {

            attribute_descriptions.push_back({
                ++next_location, 0,
                VK_FORMAT_R32G32_SFLOAT,
                offsetof(Vulkan_Vertex, tex_coor)
            });

            break;
        }
        default: 
        {
            break;
        }
    }

    attribute_descriptions.push_back({
        ++next_location, 1,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        0
    });

    attribute_descriptions.push_back({
        ++next_location, 1,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        sizeof(glm::vec4)
    });

    attribute_descriptions.push_back({
        ++next_location, 1,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        sizeof(glm::vec4) * 2
    });

    attribute_descriptions.push_back({
        ++next_location, 1,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        sizeof(glm::vec4) * 3
    });

    return attribute_descriptions;
}