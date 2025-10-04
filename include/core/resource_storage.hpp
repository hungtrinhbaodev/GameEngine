#pragma once
#include <map>
#include <thread>
#include <memory> 
#include <mutex>
#include <functional>
#include <type_traits>
#include <core/resource.hpp>
#include <utility/log_utils.h>


namespace Core {

    enum Resource_Load_Mode {
        SYNC,
        ASYNC
    };
    
    /**
     * K {Key} is class using to key to storage the resource
     * RE {Resource} is class extend from Resource
     * RD {Resource_Description_Param} is class contain the resource creation params
     * SE {Storage_Resource_Extend} is class extend from Resource_Storage
     */
    template <typename K, typename RE, typename RD, typename SE>
    class Resource_Storage {

        static_assert(
            std::is_base_of<Resource, RE>::value, 
            "Error: ResourceManager<K, RE, RD, SE> requires R to inherit from Resource."
        );

        using Resource_Loaded_Callback = std::function<void(RE*)>;

        protected:

        std::mutex _load_lock;

        std::map<K, RE*> _resources;

        std::map<K, std::vector<Resource_Loaded_Callback>> _callbacks;

        void _do_callbacks(const K& key, bool is_lock = true) {
            if (is_lock) {
                _load_lock.lock();
            }

            if (_callbacks.find(key) != _callbacks.end()) {
                for (const auto& callback : _callbacks[key]) {
                    if (callback != nullptr)
                        callback(_resources[key]);
                }
            }
            _callbacks.erase(key);

            if(is_lock) {
                _load_lock.unlock();
            }
        }

        public:

        /**
         * resource storage extend class need overidde this function to load the resource
         */
        virtual void _load_resource(
            RE* resource,
            const RD& load_params
        ) {
            
        }

        RE* load_resource(
            const Resource_Load_Mode& load_mode, 
            const K& key,
            const RD& load_params,
            Resource_Loaded_Callback callback = nullptr
        ) {

            Utility::Log::get()->log_info("go here load 0", key);
            _load_lock.lock();

            // in case resource is unload before
            if (_resources.find(key) == _resources.end()) {
                Utility::Log::get()->log_info("go here load 0.1", key);
                RE* resoruce = new RE();
                _resources[key] = resoruce;
                _callbacks[key] = {};
            }

            // check state and do resource load callback
            RE* resource = _resources[key];
            Utility::Log::get()->log_info("go here load 1", key, resource->get_loaded_state());
            switch(resource->get_loaded_state()) {
                case Resource_Loaded_State::UN_LOAD: {
                    Utility::Log::get()->log_info("go here load 2", key);
                    _callbacks[key].push_back(callback);
                    resource->set_loaded_state(Resource_Loaded_State::LOADING);
                    switch(load_mode) {
                        case Resource_Load_Mode::ASYNC: {
                            std::thread t {
                                [this, key, resource, load_params]() {
                                    SE& storage = static_cast<SE&>(*this);
                                    storage._load_resource(
                                        resource,
                                        load_params
                                    );
                                    resource->on_resource_loaded_finish();
                                    _do_callbacks(key);
                                }
                            };
                            t.detach();
                            break;
                        }
                        default: {
                            Utility::Log::get()->log_info("go here load 3", key);
                            _load_resource(
                                resource,
                                load_params
                            );
                            resource->on_resource_loaded_finish();
                            Utility::Log::get()->log_info("go here do callback 1", _callbacks[key].size());
                            _do_callbacks(key, false);
                            Utility::Log::get()->log_info("go here do callback 2");
                            break;
                        }
                    }
                    break;
                }
                case Resource_Loaded_State::LOADING: {
                    switch (load_mode) {
                        case Resource_Load_Mode::ASYNC: {
                            _callbacks[key].push_back(callback);
                            break;
                        }
                        case Resource_Load_Mode::SYNC: {
                            _callbacks[key].push_back(callback);
                            while (resource->get_loaded_state() != Resource_Loaded_State::LOADED) {}
                            break;
                        }
                    }
                    break;
                }
                case Resource_Loaded_State::LOADED: {
                    if (callback != nullptr) {
                        callback(resource);
                    } 
                    break;
                }
                default: {
                    break;
                }
            }

            _load_lock.unlock();

            return resource;
        }

        RE* get_template_resource(const K key) {

            _load_lock.lock();

            if (_resources.find(key) != _resources.end()) {
                _load_lock.unlock();
                return _resources[key];
            }

            _resources[key] = new RE();
            if (_callbacks.find(key) == _callbacks.end()) {
                _callbacks[key] = {}; 
            }

            _load_lock.unlock();

            return _resources[key];
        }

        void destroy_resources() {

            _load_lock.lock();

            _callbacks.clear();

            for (const auto& [key, resource] : _resources) {
                resource->destroy();
                delete(resource);
            }

            _resources.clear();

            _load_lock.unlock();

        }

        virtual ~Resource_Storage() = default;
    };
}