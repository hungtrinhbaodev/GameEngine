#pragma once

#include <mutex>
#include <vector>

template <typename T> class Concurent_Pool {

  protected:
	std::mutex _pool_lock;

	std::vector<T> _all_items_created;

	std::vector<T> _pool;

	virtual T _create_item() { return T{}; }

	virtual void _delete_item(T& item) {}

  public:
	T request_item() {
		T found;
		{
			std::unique_lock<std::mutex> lock(_pool_lock);
			if (_pool.size() <= 0) {
				T item = _create_item();
				_pool.push_back(item);
				_all_items_created.push_back(item);
			}
			found = _pool.back();
			_pool.pop_back();
		}
		return found;
	}

	void pooling_item(const T& item) {

		std::unique_lock<std::mutex> lock(_pool_lock);

		_pool.push_back(item);
	}

	void destroy() {
		for (T& item : _all_items_created) {
			_delete_item(item);
		}
	}

	virtual ~Concurent_Pool() {

	};
};