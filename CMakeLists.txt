cmake_minimum_required(VERSION 3.20)
project(GameEngine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)

# Per-machine dependency locations. Override via -D on the CMake command line,
# or by setting the matching environment variable before configuring.
if(NOT GLFW_DIR AND DEFINED ENV{GLFW_DIR})
    set(GLFW_DIR "$ENV{GLFW_DIR}")
endif()
if(NOT GLFW_DIR)
    message(FATAL_ERROR "GLFW_DIR is not set. Pass -DGLFW_DIR=<path to glfw-3.4.bin.WIN64> or set the GLFW_DIR environment variable.")
endif()

if(NOT GLM_DIR AND DEFINED ENV{GLM_DIR})
    set(GLM_DIR "$ENV{GLM_DIR}")
endif()
if(GLM_DIR)
    add_subdirectory(${GLM_DIR} ${CMAKE_BINARY_DIR}/glm_build)
endif()

file(GLOB_RECURSE SOURCES "main.cpp" "src/*.cpp")
add_executable(GameEngine ${SOURCES})

target_include_directories(GameEngine PRIVATE src vendor vendor/tinygltf "${GLFW_DIR}/include")

if(MINGW)
    set(GLFW_LIB "${GLFW_DIR}/lib-mingw-w64/libglfw3.a")
else()
    set(GLFW_LIB "${GLFW_DIR}/lib-vc2022/glfw3.lib")
endif()

target_link_libraries(GameEngine PRIVATE Vulkan::Vulkan "${GLFW_LIB}")
if(TARGET glm::glm)
    target_link_libraries(GameEngine PRIVATE glm::glm)
endif()

if(DEFINED ENV{PYTHON_EXEC_DIR})
    set(PYTHON_EXEC_DIR "$ENV{PYTHON_EXEC_DIR}")
endif()

find_program(PYTHON_EXEC
    NAMES python python3
    PATHS ${PYTHON_EXEC_DIR}
)

if(PYTHON_EXEC)
    add_custom_command(TARGET GameEngine POST_BUILD
        COMMAND ${PYTHON_EXEC}
            "${CMAKE_SOURCE_DIR}/scripts/copy_res.py"
            "${CMAKE_SOURCE_DIR}/res"
            "$<TARGET_FILE_DIR:GameEngine>/res"
        COMMENT "Syncing res/ to build output..."
    )
else()
    message(WARNING "Python not found — res/ will not be copied to build output.")
endif()
