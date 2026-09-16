#pragma once
#include <vector>
#include <fstream>
#include <string>
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

namespace Files {

	inline void assert_file(const std::string& file_path) {
		if (!fs::exists(file_path)) {
			std::string msg = std::string("fail to load file: ") + file_path + " does not exist!";
			throw std::runtime_error(msg);
		}
	}

	inline std::vector<char> read_file(std::string file_path, int read_mode = std::ios::ate | std::ios::binary) {

		assert_file(file_path);

		std::ifstream file(file_path, static_cast<std::ios::openmode>(read_mode));

		if (!file.is_open()) {
			std::string msg = std::string("fail to load file: ") + file_path + " fail to open!";
			throw std::runtime_error(msg);
		}

		size_t size = file.tellg();
		std::vector<char> buffer(size);
		file.seekg(0);
		file.read(buffer.data(), size);

		return buffer;
	}

} // namespace Files