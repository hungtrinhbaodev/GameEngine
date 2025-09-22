#include <utility/log_utils.h>

Utility::Log* Utility::Log::_instance = nullptr;

const char* Utility::Log::SEPARATE = "   ";

std::mutex Utility::Log::_instance_lock;

Utility::Log* Utility::Log::get() {
    _instance_lock.lock();

    if (_instance == nullptr) {
        _instance = new Log();
    }

    _instance_lock.unlock();

    return _instance;
}

void Utility::Log::clear() {
    _instance_lock.lock();

    delete(_instance);
    _instance = nullptr;

    _instance_lock.unlock();
}