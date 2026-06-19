cmake_minimum_required(VERSION 3.20)
project(GameEngine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
set(GLFW_DIR "C:/Tool/glfw-3.4.bin.WIN64/")

file(GLOB_RECURSE SOURCES "main.cpp" "src/*.cpp")
add_executable(GameEngine ${SOURCES})

target_include_directories(GameEngine PRIVATE src vendor vendor/tinygltf "${GLFW_DIR}/include")
target_link_libraries(GameEngine PRIVATE Vulkan::Vulkan "${GLFW_DIR}/lib-vc2022/glfw3.lib")