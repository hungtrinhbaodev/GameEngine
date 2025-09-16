#pragma once
#include <iostream>
#include <glm/glm.hpp>

namespace Utility {
    class GlmUtils {
        public:

        template <typename T>
        static void LOG_MAT(const T &mat) {
            static_assert(
                std::is_same<T, glm::mat2>::value ||
                std::is_same<T, glm::mat3>::value ||
                std::is_same<T, glm::mat4>::value,
                "LOG_MAT require glm::mat2, glm::mat3, glm::mat4 type!"
            );
            int cols = T::length();
            int rows = T::length();
            for (int i = 0; i < rows; i++) {
                for(int j = 0; j < cols; j++) {
                    std::cout << mat[j][i] << " ";
                }
                std::cout << std::endl;
            }
        }

        template <typename T>
        static void LOG_VEC(const T &vec) {
            static_assert(
                std::is_same<T, glm::vec1>::value ||
                std::is_same<T, glm::vec2>::value ||
                std::is_same<T, glm::vec3>::value ||
                std::is_same<T, glm::vec4>::value,
                "LOG_VEC require glm::vec1, glm::vec2, glm::vec3, glm::vec4 type!"
            );
            int cols = T::length();
            for (int i = 0; i < cols; i++) {
                std::cout << vec[i] << std::endl;
            }
        }

    };
}

