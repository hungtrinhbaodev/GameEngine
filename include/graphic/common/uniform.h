#pragma once
#define GLM_FORCE_RADIANS
#include<glm/glm.hpp>

namespace Graphic {

    class Uniform {

        public:

        glm::mat4 model;

        glm::mat4 view;
        
        glm::mat4 projection;

    };
}