#pragma once
#include <map>
#include <functional>

template<typename Key, typename Value>
struct Concurrent_Chache {

    enum Load_State {
        WAITING,
        LOADING,
        LOADED
    }

    struct Stored_Info {
        Load_State load_state;
        std::shared_ptr<Value> value;
    }

    std::mutex load_lock;

    std::map<Key, Stored_Info> resources;

};

