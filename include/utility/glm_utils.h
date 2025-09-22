#pragma once
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

template<glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
std::ostream& operator<<(std::ostream& os, const glm::mat<C, R, T, Q>& matrix) {
    std::cout << std::endl;
    for (glm::length_t i = 0; i < R; ++i) {
        for (glm::length_t j = 0; j < R; ++j) {
            os << matrix[j][i] << " ";
        }
        if (i < R - 1) {
            os << std::endl;
        }
    }
    return os;
}

template<glm::length_t C, typename T, glm::qualifier Q>
std::ostream& operator<<(std::ostream& os, const glm::vec<C, T, Q>& vec) {
    std::cout << std::endl;
    for (glm::length_t i = 0;i < C; i++) {
        os << vec[i];
        if (i < C - 1) {
            os << std::endl;
        }
    }
    return os;
}

namespace Utility {

    class Glm {
        
        public:
        
        static glm::mat4 make_scale(float scale_x = 1.0f, float scale_y = 1.0f, float scale_z = 1.0f);
        
        static glm::mat4 make_translation(float dx = 0.0f, float dy = 0.0f, float dz = 0.0f);

        static glm::mat4 make_rotation(float rx = 0.0f, float ry = 0.0f, float rz = 0.0f);

    };
};