#pragma once
#include <mutex>
#include <stack>

struct Id_Generator {
	bool is_concurent = false;
	std::mutex gen_lock;
	uint32_t counter_id = 0;
	std::stack<uint32_t> available_ids;

	uint32_t gen_id() {
		auto get_id = [this]() {
			if (available_ids.size() > 0) {
				uint32_t id = available_ids.top();
				available_ids.pop();
				return id;
			}
			return counter_id++;
		};
		if (is_concurent) {
			std::unique_lock<std::mutex> lock(gen_lock);
			return get_id();
		}
		return get_id();
	}

	void release_id(uint32_t id) {
		if (is_concurent) {
			std::unique_lock<std::mutex> lock(gen_lock);
			available_ids.push(id);
			return;
		}
		available_ids.push(id);
	}
};