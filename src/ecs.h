#pragma once
#include <cstdint>
#include <memory>
#include <sparse_set.h>
#include <tuple>
#include <vector>

inline uint32_t next_type_id() {
	static uint32_t counter = 0;
	return counter++;
};

template <typename T> uint32_t type_id() {
	static const uint32_t id = next_type_id();
	return id;
}

struct I_Pool {
	virtual ~I_Pool() = default;
	virtual void erase(uint32_t e) = 0;
};

template <typename T> struct Pool : I_Pool {
	Sparse_Set<T> set;
	void erase(uint32_t e) override { set.erase(e); }
};

class Registry {
	std::vector<std::unique_ptr<I_Pool>> pools;

	template <typename T> Sparse_Set<T>& pool() {
		uint32_t id = type_id<T>();
		if (id >= pools.size()) {
			pools.resize(id + 1);
		}
		if (!pools[id]) {
			pools[id] = std::make_unique<Pool<T>>();
		}
		return static_cast<Pool<T>*>(pools[id].get())->set;
	}

  public:
	template <typename T> bool has(uint32_t e) { return pool<T>().has(e); }

	template <typename... Ts> auto has_all(uint32_t e) { return (pool<Ts>().has(e) && ...); }

	template <typename T> void add(uint32_t e, T value) { pool<T>().insert(e, std::move(value)); }

	template <typename... Ts> auto get(uint32_t e) { return std::tie(pool<Ts>().modify(e)...); }

	template <typename... Ts> auto read(uint32_t e) { return std::tie(pool<Ts>().get(e)...); }

	template <typename T> void erase(uint32_t e) { pool<T>().erase(e); };

	template <typename First, typename... Rest, typename Fn> void each(Fn&& fn) {
		auto& first = pool<First>();
		for (uint32_t e : first.keys()) {
			if ((pool<Rest>().has(e) && ...)) {
				fn(e, first.modify(e), pool<Rest>().modify(e)...);
			}
		}
	}
};
