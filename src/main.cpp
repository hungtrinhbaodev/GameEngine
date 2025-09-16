#include <iostream>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/matrix.hpp>
#include <thread>
#include <utility/glm_utils.h>

int main() {
    glm::mat3 m(2);
    std::cout << m << std:: endl;
    glm::vec3 v(2);
    std::cout << v << std:: endl;
    std::cout << m * v << std::endl;
    return 0;
}