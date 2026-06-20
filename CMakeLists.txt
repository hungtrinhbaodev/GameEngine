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

find_program(PYTHON_EXEC
    NAMES python python3
    PATHS "C:/Users/Hung/AppData/Local/Programs/Python/Python313"
    NO_DEFAULT_PATH
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