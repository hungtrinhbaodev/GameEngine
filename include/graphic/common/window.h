#pragma once
#include <GLFW/glfw3.h>

#include <mutex>

#include <utility/log_utils.h>

namespace Graphic {

    class Window {
        private:

        static std::mutex _instance_lock;

        static Window* _instance;

        GLFWwindow* _window;

        const char* _window_name = "";

        int _width = 0;
        
        int _height = 0;

        public:

        Window();

        void init_window(const char* window_name, int width, int height);

        GLFWwindow *get_window();

        bool is_running();

        void clear_window();

        static Window* get();
    };
}