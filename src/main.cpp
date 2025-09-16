#include <iostream>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <thread>
#include <utility/glm_utils.h>

int main() {
    glm::mat4 scale = Utility::Glm::make_scale(1.5f, 1.2f, 0.5f);
    glm::mat4 rotation = Utility::Glm::make_rotation(60.f, 60.f, 45.f);
    std::cout << scale << std::endl;
    std::cout << rotation << std::endl;
    return 0;
}