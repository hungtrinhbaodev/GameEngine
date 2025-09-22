#pragma once
#include <iostream>
#include <string>
#include <type_traits>
#include <mutex>

#include <utility/glm_utils.h>

namespace Utility {

    class Log {

        private:

        static Log* _instance;

        static const char* SEPARATE;

        static std::mutex _instance_lock;

        template<typename T>
        void _log_arg(const T& arg) {
            if constexpr (std::is_convertible_v<decltype(std::cout << std::declval<T>()), std::ostream&>) {
                std::cout << arg;
            }
            else {
                std::cout << "<unprintable type>";
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
