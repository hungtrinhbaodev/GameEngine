#pragma once
#include <vector>
#include <mutex>

namespace Core {
    
    template<typename T>
    class Concurent_Pool {
        
        protected:

        std::mutex _lock_pool;

        std::vector<T> _all_items_created;

        std::vector<T> _pool;

        virtual T _create_item() {
            return T{};
        }

        virtual void _delete_item(T& item) {

        }

        public:

        T& request_item() {

            _lock_pool.lock();

            if (_pool.size() <= 0) {
                T item = _create_item();
                _pool.push_back(item);
                _all_items_created.push_back(item);
            }

            T& item = _pool.back();
            _pool.pop_back();

            _lock_pool.unlock();

            return item;
        }

        void pooling_item(const T& item) {
            
            _lock_pool.lock();

            _pool.push_back(item);

            _lock_pool.unlock();
        }

        void destroy() {
            for (T& item : _all_items_created) {
                _delete_item(item);
            }
        }

        virtual ~Concurent_Pool() {

        };

    };
}