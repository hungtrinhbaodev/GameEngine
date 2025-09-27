#pragma once

#include <mutex>
#include <thread>

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

        public: 

        static void init(Window* window);

        static void run(Window* window);

        static void main(Window* window);

        static void destroy();

        static bool is_running();

    };

}