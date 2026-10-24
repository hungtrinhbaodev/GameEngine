#pragma once
#include <handle.h>
#include <mutex>
#include <sparse_set.h>
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

	void clear() {
		if (is_concurent) {
			std::unique_lock<std::mutex> lock(gen_lock);
			counter_id = 0;
			available_ids = std::stack<uint32_t>();
			return;
		}
		counter_id = 0;
		available_ids = std::stack<uint32_t>();
	}
};

struct Handle_Generator {

	Id_Generator id_generator{};

	Sparse_Set<uint32_t> generations{};

	inline Handle gen_handle() {
		uint32_t id = id_generator.gen_id();
		uint32_t generation = 0;
		if (!generations.has(id)) {
			generations.insert(id, generation);
		} else {
			generation = generations.get(id);
			generations.insert(id, generation++);
		}
		return Handle{id, generation};
	}

	inline void release_handle(const Handle& handle) {
		if (!validate_handle(handle)) {
			return;
		}
		id_generator.release_id(handle.id());
	}

	inline bool validate_handle(const Handle& handle) {
		if (!generations.has(handle.generation())) {
			return false;
		}
		return true;
	}
};