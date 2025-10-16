#pragma once
#include <mutex>
#include <queue>
#include <iostream>

namespace Core {

    template<typename ID>
    class Objects_ID_Generated {

        protected:

        bool is_init = false;

        std::mutex _gen_lock;

        std::queue<ID> _un_used_ids;

        ID _counter;

        virtual ID _inscrease_id(ID counter) {
            // make id inscrease up or make some hash to gen id
            return counter;
        }

        virtual void init() {

        }

        public:

        ID gen_id() {
            std::unique_lock<std::mutex> lock(_gen_lock);
            
            if (!is_init) {
                init();
                is_init = true;
            }

            ID _id;
            if (_un_used_ids.size() > 0) {
                _id = _un_used_ids.front();
                _un_used_ids.pop();
            }
            else {
                _id = _counter;
                _counter = _inscrease_id(_counter);
            }
            return _id;
        }

        void free_id(const ID& id) {
            std::unique_lock<std::mutex> lock(_gen_lock);
            _un_used_ids.push(id);
        }

        virtual ~Objects_ID_Generated() {

        }

    };

    class Longs_ID_Generated : public Objects_ID_Generated<long> {

        protected:

        void init() {
            std::cout << "Go here init counter 1" << _counter << std::endl;
            _counter = 0;
            std::cout << "Go here init counter 2" << _counter << std::endl;
        }

        long _inscrease_id(long counter) {
            counter++;
            return counter;
        }

    };
}