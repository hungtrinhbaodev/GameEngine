#pragma once
#include <chrono>
#include <filesystem>
#include <log.h>
#include <string>
#include <vector>
#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif
#include <glm/glm.hpp>

namespace glm {
	inline mat4 make_mat4_from(const std::vector<float>& flat_cols) {
		mat4 result{};
		for (int i = 0; i < 4; i++) {
			vec4& col = result[i];
			int start = i * 4;
			for (int j = 0; j < 4; j++) {
				col[j] = flat_cols[start + j];
			}
		}
		return result;
	}
	template <length_t C, qualifier Q = defaultp>
	inline vec<C, float, Q> make_vec_from(const std::vector<float>& floats) {
		vec<C, float, Q> result{0.f};
		for (int i = 0; i < C; i++) {
			result[i] = floats[i];
		}
		return result;
	}
	inline glm::mat4 make_mat4_from(const float* flat_float) {
		glm::mat4 result{0.f};
		for (int i = 0; i < 4; i++) {
			vec4& col = result[i];
			int start = i * 4;
			for (int j = 0; j < 4; j++) {
				col[j] = flat_float[start + j];
			}
		}
		return result;
	}
} // namespace glm

namespace Utils {

	inline std::string DEFAULT_PATH = "";
	/**
	 * @Note: 0 is not calculate, 1 is calculated and true, -1 is calculated and false
	 */
	inline int IS_LITTLE_EDIAN = 0;

	template <typename T>
	inline std::vector<T> parse_char(std::vector<char> data, uint32_t size, uint32_t offset_byte = 0) {
		std::vector<T> res_data{};
		uint32_t stride = sizeof(T);
		if (data.size() < stride * size)
			return res_data;
		for (int i = 0; i < size; i++) {
			T* parse_data = reinterpret_cast<T*>(data.data() + offset_byte + i * stride);
			res_data.push_back(*parse_data);
		}
		return res_data;
	}

	inline std::filesystem::path get_root() {
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

	inline std::string get_root_path() {
		if (DEFAULT_PATH == "") {
			DEFAULT_PATH = get_root().string();
		}
		return DEFAULT_PATH + "/";
	}

	inline double now() {
		using Clock = std::chrono::steady_clock;
		static const Clock::time_point start = Clock::now();
		return std::chrono::duration<double>(Clock::now() - start).count();
	}

	inline bool is_little_endian() {
		if (IS_LITTLE_EDIAN == 0) {
			/**
			 * Calculate little edian
			 */
			uint32_t value = 1;
			uint8_t first_bytes;
			memcpy(&first_bytes, &value, sizeof(uint8_t));
			IS_LITTLE_EDIAN = first_bytes ? 1 : -1;
		}
		return IS_LITTLE_EDIAN == 1;
	}

	template <typename T> std::string to_bit(void* data, uint32_t offset = 0) {
		std::string bits;
		int bytes_size = sizeof(T);
		for (int i = 0; i < bytes_size; i++) {
			char byte;
			memcpy(&byte, (char*)data + offset + i, 1);
			unsigned char byte_compare = 128;
			std::string bits_at_byte;
			for (int j = 0; j < 8; j++) {
				int index = i * 8 + j;
				bits_at_byte.push_back(((1 << (7 - j)) & byte) ? '1' : '0');
			}
			if (i < bytes_size - 1) {
				bits += bits_at_byte + ".";
			} else {
				bits += bits_at_byte;
			}
		}
		return "{" + bits + "}";
	}

	inline uint32_t parse_uint32_t(void* data, uint32_t offset = 0) {
		uint32_t result;
		memcpy(&result, (char*)data + offset, sizeof(uint32_t));
		if (is_little_endian()) {
			return result;
		}
		char* end = (char*)(&result) + sizeof(uint32_t) - 1;
		uint32_t big_edian_result = 0;
		uint32_t size_bit = sizeof(uint32_t) * 8;
		for (uint32_t i = 0; i < size_bit; i++) {
			uint32_t reverse = (size_bit - 1 - i);
			uint32_t little_endian_bits = (result << i) >> reverse;
			uint32_t bit_at_big_endian = (1 >> reverse) & little_endian_bits;
			big_edian_result |= bit_at_big_endian;
		}
		for (int i = 0; i < sizeof(uint32_t); i++) {
			memcpy(end - i, (char*)(&big_edian_result) + i, 1);
		}
		return big_edian_result;
	}

	inline std::string get_folder_path_from(const std::string& file_path) {
		int last_backslash = file_path.find_last_of("/");
		return file_path.substr(0, last_backslash + 1);
	}

	inline const std::string BASE64_ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	inline std::unordered_map<char, uint32_t> decode_map{};

	inline std::mutex decode_map_mutex{};

	inline std::vector<char> base64_to_bytes(std::string_view data) {
		if (data.size() <= 0) {
			return {};
		}
		{
			std::unique_lock<std::mutex> lock(decode_map_mutex);
			if (decode_map.size() <= 0) {
				for (uint32_t i = 0; i < BASE64_ALPHABET.size(); i++) {
					decode_map[BASE64_ALPHABET[i]] = i;
				}
			}
		}
		std::vector<char> bytes;
		std::array<char, 3> bytes_at_pattern{};
		uint32_t filter = 0;
		for (int i = 0; i < 8; i++) {
			filter |= (1 << i);
		}
		int number_padding = 0;
		int last_index = data.size() - 1;
		while (last_index >= 0 && data[last_index] == '=') {
			last_index--;
			number_padding++;
		}
		if (last_index < 0) {
			return {};
		}
		for (int i = 0; i < data.size() / 4; i++) {
			int start = i * 4;
			int end = start + 4;
			uint32_t pattern = 0;
			for (int j = start; j < end && j <= last_index; j++) {
				const auto it = decode_map.find(data[j]);
				if (it == decode_map.end()) {
					throw std::runtime_error("Fail to parse base64 into bytes: not found char in data!");
				}
				int shift = (end - j - 1) * 6;
				pattern |= (it->second << shift);
			}
			for (int j = 0; j < 3; j++) {
				int byte_index = 3 - j - 1;
				uint32_t value = pattern & filter;
				bytes_at_pattern[byte_index] = (char)value;
				pattern >>= 8;
			}
			if (i >= data.size() / 4 - 1) {
				for (int j = 0; j < bytes_at_pattern.size() - number_padding; j++) {
					bytes.push_back((char)bytes_at_pattern[j]);
				}
			} else {
				for (int j = 0; j < bytes_at_pattern.size(); j++) {
					bytes.push_back((char)bytes_at_pattern[j]);
				}
			}
		}
		return bytes;
	}

} // namespace Utils