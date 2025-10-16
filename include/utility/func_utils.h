#pragma once
#include <iostream>
#include <vector>
#include <utility/log_utils.h>

namespace Utility {

    class Func_Utils {

        public:

        template<typename T>
        static void swap(T& a, T& b) {
            T tmp = std::move(b);
            b = a;
            a = tmp;
        }

        template<typename T>
        static std::vector<T> parse_data(void* data, uint32_t offset, uint32_t size) {
            std::vector<T> parse_data;
            uint8_t* cp = static_cast<uint8_t*>(data) + offset;
            for (size_t i = 0; i < size; i++) {
                uint8_t temp_byte = cp[i];
                // do something with temp_byte or just operate on cp[i] direcctly.
                parse_data.emplace_back(static_cast<T>(temp_byte));
            }
            return parse_data;
        }

        template<typename T>
        static std::vector<T> revert_data(const std::vector<uint8_t>& parse_data) {
            size_t stride = sizeof(T);
            std::vector<T> ret;
            ret.reserve(parse_data.size() / stride);
            Utility::Log::get()->log_info("parse_data.size() / stride", parse_data.size(), stride, parse_data.size() / stride);
            for (int i = 0;i < parse_data.size() / stride; i++) {
                std::vector<uint8_t> byte_in_stride;
                byte_in_stride.reserve(stride);
                int start = i * stride;
                for (int j = 0;j < stride;j++) {
                    byte_in_stride.emplace_back(parse_data[start + j]);
                }
                T* value = new T();
                memcpy(value, byte_in_stride.data(), stride);
                ret.emplace_back(*value);
                delete(value);
            }
            return ret;
        }
    };

}