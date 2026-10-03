#pragma once
#include <algorithm>
#include <id_generator.h>
#include <set>
#include <stdexcept>
#include <vector>

template <typename T> class Sparse_Set {
  private:
	Id_Generator id_generator{};
	std::vector<uint32_t> ids;
	std::vector<T> dense;
	std::vector<int> id_to_index;
	std::vector<uint32_t> index_to_id;
	float size_factor = 1.5f;

  public:
	uint32_t insert(const T& data) {
		uint32_t id = id_generator.gen_id();
		if (id_to_index.size() <= id + 1) {
			uint32_t new_size = (uint32_t)((id + 1) * size_factor);
			id_to_index.resize(new_size, -1);
			index_to_id.resize(new_size, 0);
		}
		dense.push_back(data);
		ids.push_back(id);
		index_to_id[dense.size() - 1] = id;
		id_to_index[id] = dense.size() - 1;
		return id;
	}

	bool has(uint32_t id) {
		if (id >= id_to_index.size())
			return false;
		return id_to_index[id] != -1;
	}

	const T& get(uint32_t id) {
		if (!has(id)) {
			throw std::runtime_error("Fail to get value with id: " + std::to_string(id) + "!");
		}
		return dense[id_to_index[id]];
	}

	const std::vector<uint32_t>& keys() { return ids; }

	const std::vector<T>& values() { return dense; }

	void erase(uint32_t id) {
		if (!has(id)) {
			return;
		}
		int index = id_to_index[id];
		int last_index = dense.size() - 1;
		int last_id = index_to_id[last_index];
		ids[index] = last_id;
		dense[index] = dense.back();
		id_to_index[last_id] = index;
		index_to_id[index] = last_id;
		id_to_index[id] = -1;
		index_to_id[last_index] = -1;
		dense.pop_back();
		ids.pop_back();
		id_generator.release_id(id);
	}
};