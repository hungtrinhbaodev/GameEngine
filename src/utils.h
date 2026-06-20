#pragma once
#include <vector>
#include <string>
#include <filesystem>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace Utils {

	std::string inline DEFAULT_PATH = "";

	template <typename T> inline std::vector<T> parse_char(std::vector<char> data, uint32_t size) {
		std::vector<T> res_data{};
		uint32_t stride = sizeof(T);
		if (data.size() < stride * size)
			return res_data;
		for (int i = 0; i < size; i++) {
			T* parse_data = reinterpret_cast<T*>(data.data() + i * stride);
			res_data.push_back(*parse_data);
		}
		return res_data;
	}

	std::filesystem::path inline get_root() {
#if defined(_WIN32)
		wchar_t buffer[MAX_PATH];
		DWORD length = GetModuleFileNameW(NULL, buffer, MAX_PATH);
		if (length <= 0) {
			throw std::runtime_error("Fail to get root path with win32!");
		}
		return std::filesystem::path{buffer}.parent_path();
#elif defined(__APPLE__)
		uint32_t buffer_size = 0;
		_NSGetExecutablePath(nullptr, &buffer_size);
		std::vector<char> raw_path(buffer_size);
		if (_NSGetExecutablePath(raw_path.data(), &buffer_size) != 0) {
			throw std::runtime_error("Fail to get root path with apple!");
		}
		return std::filesystem::path{raw_path.data()}.parent_path();
#else
		return std::filesystem::canonical("/proc/self/exe").parent_path();
#endif
	}

	std::string inline get_root_path() {
		if (DEFAULT_PATH == "") {
			DEFAULT_PATH = get_root().string();
		}
		return DEFAULT_PATH + "\\";
	}

} // namespace Utils