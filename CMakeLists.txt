cmake_minimum_required(VERSION 3.10)

add_definitions(-std=c++17)

# Set the project name
project(MyProject)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CXX_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS ${CXX_FLAGS})

# Define the source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Add executable target
add_executable(main ${SOURCES})

target_include_directories(main PRIVATE include)

target_include_directories(main
    PRIVATE
    dependencies
)

# Specify the path to the Vulkan library directory
set(VULKAN_LIBRARY_PATH "usr/local/lib/")

# Specify the path to the GLM library directory
set(GLM_LIBRARY_PATH "usr/local/Cellar/glm/1.0.1/lib")

target_link_libraries(main
    glfw
    "-Wl,-rpath,/${VULKAN_LIBRARY_PATH}"  vulkan
    "-Wl,-rpath,/${GLM_LIBRARY_PATH}"  glm
)
