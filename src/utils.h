#pragma once
#include <vector>

namespace Utils {

	template<typename T>
	inline std::vector<T> parse_char(std::vector<char> data, uint32_t size) {
		std::vector<T> res_data{};
		uint32_t stride = sizeof(T);
		for (int i = 0;i < size;i++) {
			T* parse_data = reinterpret_cast<T*>(data.data() + i * stride);
			res_data.push_back(*parse_data);
		}
		return res_data;
	}

}