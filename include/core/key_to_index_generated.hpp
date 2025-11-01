#pragma once
#include <map>
#include <queue>
#include <mutex>

#include <utility/log_utils.h>

namespace Core {

    template<typename Key>
    class Key_To_Index_Generated {

        protected:

        std::queue<long> _available_indices;

        std::map<Key, long> _keys_to_indices;

        long _current_index = 0;

        std::mutex _gen_lock;

        public:

        long get_index(const Key& key) {

            std::unique_lock<std::mutex> lock(_gen_lock);

            if (_keys_to_indices.find(key) == _keys_to_indices.end()) {
                Utility::Log::get()->log_info("Fail to get index, not found key!");
                return -1;
            }

            return _keys_to_indices[key];
        }

        long gen_index_by_key(const Key& key) {

            std::unique_lock<std::mutex> lock(_gen_lock);

            if (_keys_to_indices.find(key) != _keys_to_indices.end()) {
                Utility::Log::get()->log_info("Key is already generated!");
                return _keys_to_indices[key];
            }

            long index = -1;
            if (_available_indices.size() > 0) {
                index = _available_indices.front();
                _available_indices.pop();
            }
            else {
                index = _current_index;
                _current_index++;
            }
            _keys_to_indices[key] = index;

            return index;
        }


        void free_key(const Key& key) {

            std::unique_lock<std::mutex> lock(_gen_lock);

            if (_keys_to_indices.find(key) != _keys_to_indices.end()) {
                long index = _keys_to_indices[key];
                _available_indices.push(index);
                _keys_to_indices.erase(key);
            }

        }

    };

}