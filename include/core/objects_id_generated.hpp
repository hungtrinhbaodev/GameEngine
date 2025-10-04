#pragma once
#include <mutex>
#include <queue>

namespace Core {

    template<typename ID>
    class Objects_ID_Generated {

        protected:

        std::mutex _gen_lock;

        std::queue<ID> _un_used_ids;

        ID _counter;

        virtual ID _inscrease_id(ID counter) {
            // make id inscrease up or make some hash to gen id
            return counter;
        }

        public:

        ID gen_id() {
            _gen_lock.lock();
            ID _id;
            if (_un_used_ids.size() > 0) {
                _id = _un_used_ids.front();
                _un_used_ids.pop();
            }
            else {
                _id = _counter;
                _counter = _inscrease_id(_counter);
            }
            _gen_lock.unlock();
            return _id;
        }

        void free_id(const ID& id) {
            _gen_lock.lock();
            _un_used_ids.push(id);
            _gen_lock.unlock();
        }

        virtual ~Objects_ID_Generated() {

        }

    };

    class Longs_ID_Generated : public Objects_ID_Generated<long> {

        protected:

        long _inscrease_id(long counter) {
            counter++;
            return counter;
        }

    };
}