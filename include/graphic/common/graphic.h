#pragma once

#include <mutex>
#include <thread>
#include <vector>
#include <chrono>

#include <graphic/common/graphic_constants.h>
#include <graphic/vulkan_implementation/vulkan_core_data.h>
#include <utility/log_utils.h>

namespace Graphic {

    class Graphic {

        private:

        static Graphic_State _graphic_state;

        static std::mutex _graphic_state_mutex;

        static void _set_graphic_state(Graphic_State state);

        static const Graphic_State& _get_graphic_state();

        static std::vector<std::thread> threads;

        public: 

        static double global_fps;

        static double global_draw_time;

        static double global_time_prepare_data;

        static double global_time_draw_in_cpu;

        static void init(Window* window);

        static void run(Window* window);

        static void main(Window* window);

        static void destroy();

        static bool is_running();

        static void terminate();

    };

}