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

void Utility::Log::parse_void_data(const char * prefix, void* data, int size) {
    std::vector<uint32_t> parse_data;
    uint8_t* cp = static_cast<uint8_t*>(data);
    for (size_t i = 0; i < size; i++) {
        uint8_t temp_byte = cp[i];
        // do something with temp_byte or just operate on cp[i] direcctly.
        parse_data.emplace_back(temp_byte);
    }
    log_info(prefix, parse_data);
}