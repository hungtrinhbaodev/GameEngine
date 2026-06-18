#pragma once
#include <iostream>
#include <string>
#include <type_traits>
#include <mutex>
#include <vector>
#include <glm/glm.hpp>
#include <thread>

namespace Log {

	namespace {

		template <glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
		inline std::ostream& operator<<(std::ostream& os, const glm::mat<C, R, T, Q>& matrix) {
			os << std::endl;
			os << '{';
			for (glm::length_t i = 0; i < R; ++i) {
				for (glm::length_t j = 0; j < R; ++j) {
					os << matrix[j][i] << " ";
				}
				if (i < R - 1) {
					os << std::endl;
				}
			}
			os << '}';
			return os;
		}

		template <glm::length_t C, typename T, glm::qualifier Q>
		inline std::ostream& operator<<(std::ostream& os, const glm::vec<C, T, Q>& vec) {
			os << "[";
			for (glm::length_t i = 0; i < C; i++) {
				os << vec[i];
				if (i < C - 1) {
					os << ", ";
				}
			}
			os << "]";
			return os;
		}

		template <typename T> inline std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
			os << '{';
			for (size_t i = 0; i < vec.size(); i++) {
				os << vec[i];
				if (i < vec.size() - 1) {
					os << ", ";
				}
			}
			os << '}';
			return os;
		}

		inline const char* SEPARATE = " | ";

		inline std::mutex _lock_log;

		// Helper trait to check if T can be streamed to std::ostream
		template <typename T, typename = void> struct is_streamable : std::false_type {};

		template <typename T>
		struct is_streamable<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<T>())>>
			: std::true_type {};

		template <typename T> inline void _log_arg(const T& arg) {
			if constexpr (is_streamable<T>::value) {
				std::cout << arg;
			} else {
				std::cout << "<unprintable type>";
			}
		}

		template <typename T, typename... Args>
		inline void _log_info(const T& first_arg, const Args&... remaining_args) {

			_log_arg(first_arg);

			if constexpr (sizeof...(remaining_args) > 0) {
				std::cout << SEPARATE;
				_log_info(remaining_args...);
			} else {
				std::cout << std::endl;
			}
		}
	} // namespace

	// common log with ...args
	template <typename... Args> inline void log_info(const Args&... args) {

		// return;
		std::unique_lock<std::mutex> lock(_lock_log);

		std::cout << "thread id: " << std::this_thread::get_id() << SEPARATE;

		_log_info(args...);
	}

} // namespace Log