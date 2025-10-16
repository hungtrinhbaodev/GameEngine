#pragma once
#include <map>
#include <thread>
#include <memory> 
#include <mutex>
#include <functional>
#include <type_traits>
#include <core/resource.hpp>
#include <core/thread_pool.hpp>
#include <utility/log_utils.h>


namespace Core {

    enum Resource_Load_Mode {
        SYNC,
        ASYNC
    };

    enum Resource_Loaded_State {
        UN_LOAD,
        LOADING,
        LOADED
    };

    /**
     * Here is class base of resource need load in multi thread
     * Resource_Description: class or struct of params that resource need to load
     */
    template <typename Resource_Description>
    class Resource {

        private:

        Resource_Loaded_State _loaded_state = Resource_Loaded_State::UN_LOAD;

        public:

        /**
         * state of resource in loading phase will
         * be update by resource storage
         */
        void set_loaded_state(Resource_Loaded_State loaded_state) {
            _loaded_state = loaded_state;
        }

        /**
         * get state loading of resource to
         * process the callback loaded by state
         */
        Resource_Loaded_State get_loaded_state() {
            return _loaded_state;
        }

        /**
         * When resource is starting load this function will be call
         * override it when need to use LOAD START trigger
         */
        void on_start_load() {

        }

        /**
         * Main load function of resource
         * resource need override this function
         * to make the loading state
         */
        virtual void on_load(const Resource_Description& load_params) {
            // override this function to load resource with params
        }

        /**
         * When resource is loading finish this function will be call
         * override it when need to use LOAD END trigger
         */
        void on_finish_load() {
            set_loaded_state(Resource_Loaded_State::LOADED);
        }

        /**
         * When resource is destroy override this function
         * to do clean up task
         */
        template<typename ...Args>
        void destroy(const Args&... args) {

        }

        virtual ~Resource()  = default;
    };
    
    /**
     * Key: class using to key to storage the resource
     * Resource_Extend: class extend from Resource
     * Resource_Desctiprion: class contain the resource creation params
     */
    template <typename Key, typename Resource_Extend, typename Resource_Desctiprion>
    class Resource_Storage {

        static_assert(
            std::is_base_of<Resource<Resource_Desctiprion>, Resource_Extend>::value, 
            "Error: ResourceManager<Key, Resource_Extend, Resource_Desctiprion> requires Resource_Extend to inherit from Resource<Resource_Desctiprion>."
        );

        using Resource_Loaded_Callback = std::function<void(std::shared_ptr<Resource_Extend>)>;

        /**
         * When the main unique resource is loaded finish
         * this functionw will be called to do all function
         * loaded callback is cache in waiting load state
         */
        using Resource_Loaded_Main_Callback = std::function<void(const Key& key)>;

        struct Load_Resource_Task_Info {
            Key resource_key;
            std::shared_ptr<Resource_Extend> resource_need_load;
            Resource_Desctiprion resource_load_params;
            Resource_Loaded_Main_Callback resource_loaded_callback = nullptr;
        };

        class Thread_Load_Resource_Item : public Thread_Item<Load_Resource_Task_Info> {

            public:

            void do_task(Load_Resource_Task_Info task_info) {
                std::shared_ptr<Resource_Extend> resource = task_info.resource_need_load;
                const Resource_Desctiprion& load_params = task_info.resource_load_params;
                resource->on_load(load_params);
                resource->set_loaded_state(Resource_Loaded_State::LOADED);
                resource->on_finish_load();

                const Key& key = task_info.resource_key;
                auto loaded_callback = task_info.resource_loaded_callback;
                if (loaded_callback != nullptr) {
                    loaded_callback(key);
                }
            }

        };

        struct Resource_Loaded_Callback_Info {
            std::shared_ptr<Resource_Extend> resource;
            Resource_Loaded_Callback loaded_callback;
        };

        class Thread_Callback_Loaded_Item : public Thread_Item<Resource_Loaded_Callback_Info> {

            public:

            void do_task(Resource_Loaded_Callback_Info task_info) {
                std::shared_ptr<Resource_Extend> resource = task_info.resource;
                Resource_Loaded_Callback callback = task_info.loaded_callback;
                if (callback != nullptr) {
                    callback(resource);
                }
            }

        };

        protected:

        Thread_Pool<Thread_Load_Resource_Item, Load_Resource_Task_Info> _load_thread_pool;

        Thread_Pool<Thread_Callback_Loaded_Item, Resource_Loaded_Callback_Info> _callback_thread_pool;

        std::mutex _load_lock;

        std::map<Key, std::shared_ptr<Resource_Extend>> _resources;

        std::map<Key, std::vector<Resource_Loaded_Callback>> _callbacks;

        std::map<Key, long> _processing_tasks;

        void _add_callback_loaded(const Key& key, Resource_Loaded_Callback callback) {
            if (_callbacks.find(key) == _callbacks.end()) {
                _callbacks[key] = {};
            }
            _callbacks[key].emplace_back(callback);
        }

        void _do_callbacks_loaded(const Key& key) {

            std::unique_lock<std::mutex> lock(_load_lock);
            Utility::Log::get()->log_info("_do_callbacks_loaded 1", key, _callbacks.size());
            if (_callbacks.find(key) != _callbacks.end()) {
                Utility::Log::get()->log_info("_do_callbacks_loaded 2", key, _callbacks[key].size());
                for (const auto& callback : _callbacks[key]) {
                    std::shared_ptr<Resource_Extend> resource = _resources[key];
                    Resource_Loaded_Callback_Info task_info {
                        resource,
                        callback
                    };
                    _callback_thread_pool.push_task(task_info);
                }
            }
            if (_processing_tasks.find(key) != _processing_tasks.end()) {
                _processing_tasks.erase(key);
            }
            _callbacks.erase(key);
        }

        public:

        Resource_Storage() {
            _load_thread_pool.start_running(10);
            _callback_thread_pool.start_running(10);
        }

        std::shared_ptr<Resource_Extend> get_template_resource(const Key& key) {

            std::unique_lock<std::mutex> lock(_load_lock);

            std::shared_ptr<Resource_Extend> template_resoure;
            if (_resources.find(key) == _resources.end()) {
                template_resoure = std::make_shared<Resource_Extend>();
                _resources[key] = template_resoure;
            }
            else {
                template_resoure = _resources[key];
            }
            return template_resoure;

        }

        std::shared_ptr<Resource_Extend> load_resource(
            const Resource_Load_Mode& load_mode, 
            const Key& key,
            const Resource_Desctiprion& load_params,
            Resource_Loaded_Callback callback = nullptr
        ) {
            std::shared_ptr<Resource_Extend> resource = get_template_resource(key);

            std::unique_lock<std::mutex> lock(_load_lock);
            switch (resource->get_loaded_state()) {

                case Resource_Loaded_State::UN_LOAD: {
                    resource->on_start_load();
                    resource->set_loaded_state(Resource_Loaded_State::LOADING);
                    // Make a task data is using in task pool
                    Load_Resource_Task_Info task_info {
                        key,
                        resource,
                        load_params,
                        // wrapper do callback by lamda function
                        [this] (const Key& key) {
                            this->_do_callbacks_loaded(key);
                        }
                    };
                    switch (load_mode) {
                        // In sync mode if resource is never load before
                        // we load and block thread load until it load finish
                        // and do callback
                        case Resource_Load_Mode::SYNC: {
                            long task_id = _load_thread_pool.push_task(task_info);
                            lock.unlock();
                            _load_thread_pool.wait_to_task_end(task_id);
                            if (callback != nullptr) {
                                callback(resource);
                            }
                            return resource;
                        }
                        default: {
                            // In async mode if resource is never load before
                            // we load resource by thread pool and save a task id
                            // to future process
                            long task_id = _load_thread_pool.push_task(task_info);
                            _processing_tasks[key] = task_id;
                            _add_callback_loaded(key, callback);
                            break;
                        }
                    }
                    break;
                }
                case Resource_Loaded_State::LOADING: {
                    switch (load_mode) {
                        // In case sync mode we need to block thread
                        // cause resource is load in async mode before
                        // and after load done we process a callback 
                        case Resource_Load_Mode::SYNC: {
                            long task_id = _processing_tasks[key];
                            if (_processing_tasks.find(key) == _processing_tasks.end()) {
                                throw std::runtime_error("Resource is loading but not found task load id in thread pool!");
                            }
                            lock.unlock();
                            _load_thread_pool.wait_to_task_end(task_id);
                            if (callback != nullptr) {
                                callback(resource);
                            }
                            return resource;
                        }
                        default: {
                            // In case async mode we add callback
                            // to list waiting callback after 
                            // resource load finish it will be done
                            _add_callback_loaded(key, callback);
                            break;
                        }
                    }
                    break;
                }
                default: {
                    lock.unlock();
                    switch (load_mode) {
                        // In case sync mode we just do
                        // callback in this thread
                        case Resource_Load_Mode::SYNC: {
                            if (callback != nullptr) {
                                callback(resource);
                            }
                            break;
                        }
                        default: {
                            // In case async mode we just
                            // push callback to callback 
                            // thread pool and do that
                            Resource_Loaded_Callback_Info task_info {
                                resource,
                                callback
                            };
                            _callback_thread_pool.push_task(task_info);
                            break;
                        }
                    }
                    break;
                }
            }
            return resource;
        }

        size_t get_number_resource() {
            return _resources.size();
        }

        template<typename ...Args>
        void destroy_resources(const Args&... args) {

            std::unique_lock<std::mutex> lock(_load_lock);

            _load_thread_pool.destroy();
            _callback_thread_pool.destroy();
            _callbacks.clear();
            _processing_tasks.clear();
            for (const auto& [_, resource] : _resources) {
                resource->destroy(args...);
            }
            _resources.clear();
        }

        virtual ~Resource_Storage() = default;
    };
}