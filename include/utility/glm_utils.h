#pragma once
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Utility {

    class Glm {
        
        public:
        
        static glm::mat4 make_scale(float scale_x = 1.0f, float scale_y = 1.0f, float scale_z = 1.0f);
        
        static glm::mat4 make_translation(float dx = 0.0f, float dy = 0.0f, float dz = 0.0f);

        static glm::mat4 make_rotation(float rx = 0.0f, float ry = 0.0f, float rz = 0.0f);

    };
};