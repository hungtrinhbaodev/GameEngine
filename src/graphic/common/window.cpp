#include <graphic/common/window.h>

std::mutex Graphic::Window::_instance_lock;

Graphic::Window* Graphic::Window::_instance = nullptr;

Graphic::Window::Window() {

}

void Graphic::Window::init_window(const char* window_name, int width, int height) {
    this->_window_name = window_name;
    this->_width = width;
    this->_height = height;
    
    if(glfwInit()){
        Utility::Log::get()->log_info("Init GLFW success!");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    Utility::Log::get()->log_info("Create window info:", "\nWdith: " + std::to_string(_width), "\nHeight: " + std::to_string(_height));

    _window = glfwCreateWindow(_width, height, _window_name, nullptr, nullptr);
    if(_window){
        Utility::Log::get()->log_info("Create window success!");
    }
}

GLFWwindow *Graphic::Window::get_window() {
    return _window;
}

bool Graphic::Window::is_running() {
    return !glfwWindowShouldClose(_window);
}

void Graphic::Window::clear_window() {
    glfwDestroyWindow(_window);
    Utility::Log::get()->log_info("Delete glfw window success!");

    glfwTerminate();
    Utility::Log::get()->log_info("Terminate glfw success!");
}

Graphic::Window* Graphic::Window::get() {
    _instance_lock.lock();

    if (_instance == nullptr) {
        _instance = new Window();
    }

    _instance_lock.unlock();

    return _instance;
}

void Graphic::Window::clean_up() {
    _instance_lock.lock();

    delete(_instance);
    _instance = nullptr;

    _instance_lock.unlock();
}