#include <log.h>
#include <map>
#include <profiler.h>
#include <utils.h>

namespace Profiler {

	struct Profiler_Data {
		long total_call_duration = 0;
		long number_call = 0;
		long start_time_call = 0;
	};

	std::map<std::string, Profiler_Data> profilers{};

	void start_scope(std::string key) {
		if (profilers.find(key) == profilers.end()) {
			profilers[key] = {};
		}
		profilers[key].start_time_call = Utils::now();
	}

	void end_scope(std::string key) {
		if (profilers.find(key) == profilers.end()) {
			return;
		}
		profilers[key].total_call_duration += (Utils::now() - profilers[key].start_time_call);
		profilers[key].number_call++;
	}

	void view_scope(std::string key) {
		if (profilers.find(key) == profilers.end()) {
			return;
		}
		Log::info(
			"Scope " + key + " avarage time: ",
			((double)profilers[key].total_call_duration / profilers[key].number_call) / 1000
		);
	}

} // namespace Profiler