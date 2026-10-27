#pragma once
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace Files {

	inline void assert_file(const std::string& file_path) {
		if (!fs::exists(file_path)) {
			std::string msg = std::string("fail to load file: ") + file_path + " does not exist!";
			throw std::runtime_error(msg);
		}
	}

	inline bool is_file_exists(const std::string& file_path) {
		return fs::exists(file_path);
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

	inline void save_bytes_to_file(
		const std::string& file_path, const std::vector<char>& data, int write_mode = std::ios::out | std::ios::binary
	) {
		std::ofstream file(file_path, static_cast<std::ios::openmode>(write_mode));
		if (!file.is_open()) {
			std::string msg = std::string("fail to load file: ") + file_path + " fail to write!";
			throw std::runtime_error(msg);
		}
		file.write((char*)data.data(), data.size());
	}

} // namespace Files