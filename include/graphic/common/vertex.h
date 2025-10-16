#pragma once
#include <glm/glm.hpp>
#include <utility/log_utils.h>

namespace Graphic {

    class Vertex {
        
        public:

        glm::vec3 position;

        glm::vec2 tex_coor;

        glm::vec3 normal;

        glm::vec3 color;

        friend std::ostream& operator<<(std::ostream& os, const Vertex& vec) {
            os << "position: " << vec.position << ", tex_coor: " << vec.tex_coor << ", normal: " << vec.normal <<  ", color: " << vec.color << std::endl;
            return os;
        };
    };
    
}