#pragma once
#include <string>

namespace Profiler {

	void start_scope(std::string key);

	void end_scope(std::string key);

	void view_scope(std::string key);

} // namespace Profiler
