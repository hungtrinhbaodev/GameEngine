cmake_minimum_required(VERSION 3.20)
project(GameEngine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


find_package(Vulkan REQUIRED)

if(APPLE)
    # brew install glfw glm molten-vk vulkan-headers vulkan-loader
    find_package(glfw3 REQUIRED)
    find_package(glm REQUIRED)
else()
    set(GLM_DIR "D:/Dev/glm/")
    set(GLFW_DIR "D:/Dev/glfw-3.4.bin.WIN64/glfw-3.4.bin.WIN64")
    add_subdirectory(${GLM_DIR} ${CMAKE_BINARY_DIR}/glm_build)
endif()


file(GLOB_RECURSE SOURCES "main.cpp" "src/*.cpp" "tests/*.cpp")
# main.cpp runs the UI tests too, so only the test harness entry point is left out.
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/tests/test_main.cpp")
add_executable(main ${SOURCES})

target_compile_definitions(main PRIVATE $<$<CONFIG:Debug>:_DEBUG>)
target_compile_definitions(main PRIVATE $<$<CONFIG:Debug>:_GLIBCXX_ASSERTIONS>)
target_include_directories(main PRIVATE src vendor tests)

if(APPLE)
    target_link_libraries(main PRIVATE glm::glm Vulkan::Vulkan glfw)
else()
    target_include_directories(main PRIVATE "${GLFW_DIR}/include/")
    target_link_libraries(main PRIVATE glm::glm Vulkan::Vulkan "${GLFW_DIR}/lib-mingw-w64/libglfw3.a")
endif()

# UI test harness: tests/*.cpp + engine sources, without the root main.cpp.
# Build only on demand: cmake --build <dir> --target ui_test
file(GLOB_RECURSE ENGINE_SOURCES "src/*.cpp")
file(GLOB_RECURSE UI_TEST_SOURCES "tests/*.cpp")
add_executable(ui_test EXCLUDE_FROM_ALL ${UI_TEST_SOURCES} ${ENGINE_SOURCES})
target_compile_definitions(ui_test PRIVATE $<$<CONFIG:Debug>:_DEBUG>)
target_compile_definitions(ui_test PRIVATE $<$<CONFIG:Debug>:_GLIBCXX_ASSERTIONS>)
target_include_directories(ui_test PRIVATE src vendor tests)

if(APPLE)
    target_link_libraries(ui_test PRIVATE glm::glm Vulkan::Vulkan glfw)
else()
    target_include_directories(ui_test PRIVATE "${GLFW_DIR}/include/")
    target_link_libraries(ui_test PRIVATE glm::glm Vulkan::Vulkan "${GLFW_DIR}/lib-mingw-w64/libglfw3.a")
endif()

# Scene editor: editor/*.cpp + engine sources. Build: cmake --build <dir> --target editor
file(GLOB_RECURSE EDITOR_SOURCES "editor/*.cpp")
add_executable(editor EXCLUDE_FROM_ALL ${EDITOR_SOURCES} ${ENGINE_SOURCES})
target_compile_definitions(editor PRIVATE $<$<CONFIG:Debug>:_DEBUG>)
target_compile_definitions(editor PRIVATE EDITOR_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_include_directories(editor PRIVATE src vendor editor)

if(APPLE)
    target_link_libraries(editor PRIVATE glm::glm Vulkan::Vulkan glfw)
else()
    target_include_directories(editor PRIVATE "${GLFW_DIR}/include/")
    target_link_libraries(editor PRIVATE glm::glm Vulkan::Vulkan "${GLFW_DIR}/lib-mingw-w64/libglfw3.a")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND MSVC)

    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

    add_compile_definitions(_DEBUG)

    add_compile_options(/fsanitize=address /Zi)

    add_link_options(
        "clang_rt.asan_dynamic-x86_64.lib"
        "clang_rt.asan_dynamic_runtime_thunk-x86_64.lib"
    )

endif()



find_program(PYTHON_EXEC
    NAMES python python3
)

if(NOT PYTHON_EXEC)
    message(FATAL_ERROR "Python not found — required to compile shaders and copy res/ to build output.")
endif()

add_custom_target(sync_resource ALL
    COMMAND ${PYTHON_EXEC}
        "${CMAKE_SOURCE_DIR}/scripts/copy_res.py"
        "${CMAKE_SOURCE_DIR}/res"
        "$<TARGET_FILE_DIR:main>/res"
    COMMENT "Syncing res and shader/ to build output..."
)
add_dependencies(sync_resource main)