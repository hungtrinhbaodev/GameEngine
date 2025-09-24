
#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <stdexcept>
#include <utility/log_utils.h>

namespace Utility {

    class File_Utils {
        private:

            static File_Utils *_instance;

            File_Utils() {};

        public:

            static std::string DEFAULT_PATH;

            static void clean_up();

            ~File_Utils() {};

            static File_Utils *get();

            std::vector<char> read_file_shader(const std::string& path); 
    };
}
