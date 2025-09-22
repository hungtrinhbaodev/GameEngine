#pragma once
#include <iostream>
#include <string>
#include <type_traits>
#include <mutex>
#include <vector>
#include <glm/glm.hpp>

#include <utility/glm_utils.h>

template<glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
std::ostream& operator<<(std::ostream& os, const glm::mat<C, R, T, Q>& matrix) {
    os << std::endl;
    for (glm::length_t i = 0; i < R; ++i) {
        for (glm::length_t j = 0; j < R; ++j) {
            os << matrix[j][i] << " ";
        }
        if (i < R - 1) {
            os << std::endl;
        }
    }
    return os;
}

template<glm::length_t C, typename T, glm::qualifier Q>
std::ostream& operator<<(std::ostream& os, const glm::vec<C, T, Q>& vec) {
    os << std::endl;
    for (glm::length_t i = 0;i < C; i++) {
        os << vec[i];
        if (i < C - 1) {
            os << std::endl;
        }
    }
    return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
    os << '{';
    if constexpr (!std::is_convertible_v<decltype(os << std::declval<T>()), std::ostream&>) {
        os << "<unprintable type>";
    }
    else {
        for (size_t i = 0;i < vec.size(); i++) {
            os << vec[i];
            if (i < vec.size() - 1) {
                os << ", ";
            }
        }
    }
    os << '}';
    return os;
}

namespace Utility {

    class Log {

        private:

        static Log* _instance;

        static const char* SEPARATE;

        static std::mutex _instance_lock;

        template<typename T>
        void _log_arg(const T& arg) {
            if constexpr (!std::is_convertible_v<decltype(std::cout << std::declval<T>()), std::ostream&>) {
                std::cout << "<unprintable type>";
            }
            else {
                std::cout << arg;
            }
        }

        public:

        // common log with ...args 
        template<typename T, typename ...Args>
        void log_info(const T& first_arg, const Args&... remaining_args) {
            _log_arg(first_arg);

            if constexpr (sizeof...(remaining_args) > 0) {
                std::cout << SEPARATE;
                log_info(remaining_args...);
            }
            else {
                std::cout << std::endl;
            }
        }

        static Log* get();

        static void clear();
    };
}
