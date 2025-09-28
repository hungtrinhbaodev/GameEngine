#pragma once
#include <thread>
#include <map>
#include <functional>

namespace Core {

    enum Resource_Loaded_State {
        UN_LOAD,
        LOADING,
        LOADED
    };


    class Resource {

        protected:

        Resource_Loaded_State _loaded_state = Resource_Loaded_State::UN_LOAD;

        public:

        Resource_Loaded_State get_loaded_state() {
            return _loaded_state;
        }

        void set_loaded_state(Resource_Loaded_State loaded_state) {
            _loaded_state = loaded_state;
        }

        void on_resource_loaded_finish() {
            set_loaded_state(Resource_Loaded_State::LOADED);
        }

        void destroy() {
            
        }

    };

}