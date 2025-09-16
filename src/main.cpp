#include <iostream>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/matrix.hpp>
#include <thread>
#include <utility/glm_utils.h>

int main() {
    glm::mat3 m(1);
    Utility::GlmUtils::LOG_MAT(m);
    glm::vec3 v(3);
    Utility::GlmUtils::LOG_VEC(v);
    Utility::GlmUtils::LOG_VEC(m * v);
    return 0;
}