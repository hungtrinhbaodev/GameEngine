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

        private:

        std::mutex _load_lock;

        std::map<K, RE*> _resoruces;

        std::map<K, std::vector<Resource_Loaded_Callback>> _callbacks;

        void _do_callbacks(const K& key) {
            _load_lock.lock();

            if (_callbacks.find(key) != _callbacks.end()) {
                for (const auto& callback : _callbacks[key]) {
                    if (callback != nullptr)
                        callback(_resoruces[key]);
                }
            }
            _callbacks.erase(key);

            _load_lock.unlock();
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

            _load_lock.lock();

            // in case resource is unload before
            if (_resoruces.find(key) == _resoruces.end()) {
                RE* resoruce = new RE();
                _resoruces[key] = resoruce;
                _callbacks[key] = {};
            }

            // check state and do resource load callback
            RE* resource = _resoruces[key];
            switch(resource->get_loaded_state()) {
                case Resource_Loaded_State::UN_LOAD: {
                    switch(load_mode) {
                        case Resource_Load_Mode::ASYNC: {
                            resource->set_loaded_state(Resource_Loaded_State::LOADING);
                            _callbacks[key].push_back(callback);
                            std::thread t([&, key, resource, load_params]() {
                                SE& storage = static_cast<SE&>(*this);
                                storage._load_resource(
                                    resource,
                                    load_params
                                );
                                resource->on_resource_loaded_finish();
                                _do_callbacks(key);
                            });
                            t.detach();
                            break;
                        }
                        default: {
                            _load_resource(
                                resource,
                                load_params
                            );
                            resource->on_resource_loaded_finish();
                            callback(resource);
                            break;
                        }
                    }
                    break;
                }
                case Resource_Loaded_State::LOADING: {
                    _callbacks[key].push_back(callback);
                    break;
                }
                case Resource_Loaded_State::LOADED: {
                    callback(resource);
                    break;
                }
                default: {
                    break;
                }
            }

            _load_lock.unlock();

            return resource;
        }

        virtual ~Resource_Storage() = default;
    };
}