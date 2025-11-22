#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>

#ifdef __linux__
#include <execinfo.h> // For stack trace
#include <unistd.h>
#include <stdio.h> // Using C-style I/O for signal safety
#endif

#include <core/objects_id_generated.hpp>
#include <utility/func_utils.h>
#include <utility/math_utils.h>
#include <graphic/common/texture.h>
#include <graphic/common/window.h>
#include <graphic/common/graphic.h>
#include <graphic/common/mesh.h>
#include <graphic/vulkan_implementation/vulkan_mesh_buffer.hpp>
#include <graphic/vulkan_implementation/vulkan_instances_buffer.hpp>

#ifdef __linux__
const std::string DEFAULT_PATH = "/Users/lap13994/Documents/hungtrinhbaodev/GameEngine/res/texture/";
#else
const std::string DEFAULT_PATH = "D:/game_engine/game_engine/res/texture/";
#endif

#ifdef __linux__
// Change the function signature to accept siginfo_t* and void*
void signal_handler(int sig, siginfo_t* info, void* context) {
    
    // --- 1. Log the address that caused the fault (CRUCIAL) ---
    // si_addr holds the address that caused the segmentation fault.
    // This is often a null pointer address (0x0) or a bad heap address.
    fprintf(stderr, "Fatal signal %d received.\n", sig);
    fprintf(stderr, "Faulting address (si_addr): %p\n", info->si_addr);
    
    // --- 2. Print Stack Trace ---
    void* array[20];
    size_t size = backtrace(array, 20);
    fprintf(stderr, "Stack Trace:\n");
    // Using the literal '2' for STDERR_FILENO since you noted the constant issue
    backtrace_symbols_fd(array, size, 2); 

    // 3. Terminate cleanly
    signal(sig, SIG_DFL);
    raise(sig);
}
#endif

int main() {

#ifdef __linux__
    struct sigaction sa;
    
    // Use sa_sigaction and set the SA_SIGINFO flag
    sa.sa_sigaction = signal_handler; 
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO; // <-- Enables passing detailed info
    
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("Error setting signal handler for SIGSEGV");
    }
#endif

    // init window singleton
    Graphic::Window* window = Graphic::Window::get();
    window->init_window("GameEngine", 1200, 720);
    
    // init graphic with window
    Graphic::Graphic::init(window);

    // enter main thread of graphic
    Graphic::Graphic::main(window);

    // test
    const auto& data = Graphic::Vulkan_Core_Data::get();
    const auto& _vk_render_data = data->get_wrapper_data().wp_render_data;
    int size = 10000;
    for (int i = 0;i < size;i++) {
        _vk_render_data->add_model(
            "RECTANGLE",
            "",
            Utility::Glm::make_scale(0.5f, 0.5f) * Utility::Glm::make_translation(Utility::Math_Utils::rand_range(-1.0f, 1.0f), Utility::Math_Utils::rand_range(-1.0f, 1.0f))
        );
        _vk_render_data->add_model(
            "TRIANGLE",
            "",
            Utility::Glm::make_scale(0.5f, 0.5f) * Utility::Glm::make_translation(Utility::Math_Utils::rand_range(-1.0f, 1.0f), Utility::Math_Utils::rand_range(-1.0f, 1.0f))
        );
        // _vk_render_data->add_model(
        //     "RECTANGLE",
        //     "",
        //     Utility::Glm::make_scale(0.5f, 0.5f) * Utility::Glm::make_translation(Utility::Math_Utils::rand_range(-10.0f, 10.0f), Utility::Math_Utils::rand_range(-10.0f, 10.0f))
        // );
        // _vk_render_data->add_model(
        //     "TRIANGLE",
        //     "",
        //     Utility::Glm::make_scale(0.5f, 0.5f) * Utility::Glm::make_translation(Utility::Math_Utils::rand_range(-10.0f, 10.0f), Utility::Math_Utils::rand_range(-10.0f, 10.0f))
        // );
    }
    
    // _vk_render_data->add_model(
    //     "RECTANGLE",
    //     DEFAULT_PATH + "texture3.png",
    //     Utility::Glm::make_scale(0.5f, 0.5f) * Utility::Glm::make_translation(0.6f, 0.6f)
    // );
    // _vk_render_data->update_model(
    //     vk_model_info,
    //     Utility::Glm::make_scale(0.5f, 0.5f) * Utility::Glm::make_translation(0.8f, -0.8f)
    // );

    double last_time = glfwGetTime();

    // main loop of game engine
    while(Graphic::Graphic::is_running()) {
        double current_time = glfwGetTime();

        // pool user events
        glfwPollEvents();

        if (current_time - last_time >= 1.0f) {
            window->change_window_name(
                Graphic::Graphic::global_fps, 
                Graphic::Graphic::global_draw_time,
                Graphic::Graphic::global_time_prepare_data,
                Graphic::Graphic::global_time_draw_in_cpu
            );
            last_time = current_time;
        }
    }

    Graphic::Graphic::destroy();

    Graphic::Graphic::terminate();

    Utility::Log::get()->log_info("Terminate destroy all graphic processing!");

    // clear data and clean up window
    window->clear_window();
    Graphic::Window::clean_up();

    // log to finish app
    Utility::Log::get()->log_info("App is closed successfully!");

    return 0;
}