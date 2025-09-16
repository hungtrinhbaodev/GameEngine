#pragma once
#include <iostream>
#include <glm/glm.hpp>

template<glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
std::ostream& operator<<(std::ostream& os, const glm::mat<C, R, T, Q>& matrix) {
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
    for (glm::length_t i = 0;i < C; i++) {
        os << vec[i];
        if (i < C - 1) {
            os << std::endl;
        }
    }
    return os;
}