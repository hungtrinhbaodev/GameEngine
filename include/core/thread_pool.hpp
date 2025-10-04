#pragma once
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <utility>
#include <exception>
#include <map>
#include <core/objects_id_generated.hpp>
#include <utility/log_utils.h>

namespace Core {

    template<typename Task_Info>
    class Thread_Item {

        public:

        void init() {

        }

        void destroy() {

        }

        virtual void do_task(Task_Info task_info) {

        }

        virtual ~Thread_Item() {

        }

    };

    enum Thread_Task_Description {
        EMPTY,
        HAS_TASK
    };

    enum Thread_Task_State {
        NONE,
        WATIING,
        PROCESSING,
        ENDING
    };

    /**
     * Thread_Item_Extend is class extend from Thread Item
     * Task_Info is class task using in Thread Item
     */
    template<typename Thread_Item_Extend, typename Task_Info>
    class Thread_Pool {

        struct Task_Data {
            long task_id;
            Thread_Task_Description task_description;
            Task_Info task;
        };

        private:

        Longs_ID_Generated _task_id_generated;

        std::mutex _task_tracking_lock;

        std::mutex _queue_lock;

        std::mutex _init_item_lock;

        std::queue<Task_Data> _queues_task;

        std::vector<Thread_Item_Extend*> _threads_items;

        std::map<long, Thread_Task_State> _processing_tasks;

        bool _is_has_task = false;

        bool _is_running = true;

        void _add_task_tracking(long task_id, Thread_Task_State task_state) {
            _task_tracking_lock.lock();
            _processing_tasks[task_id] = task_state;
            _task_tracking_lock.unlock();
        }

        Thread_Task_State _get_task_state(long task_id) {
            _task_tracking_lock.lock();
            if (_processing_tasks.find(task_id) == _processing_tasks.end()) {
                _task_tracking_lock.unlock();
                return Thread_Task_State::NONE;
            }
            // std::cout << "_get_task_state: " << task_id << " " << _processing_tasks.size() << std::endl;
            Thread_Task_State task_state = _processing_tasks[task_id];
            _task_tracking_lock.unlock();
            return task_state;
        }

        public:

        Task_Data get_task() {

            Task_Data task {
                Thread_Task_Description::EMPTY
            };

            _queue_lock.lock();

            if (_queues_task.size() > 0) {
                task = _queues_task.front();
                _queues_task.pop();
            }

            if (_queues_task.size() <= 0) {
                _is_has_task = false;
            }

            _queue_lock.unlock();

            return task;
        }

        long push_task(Task_Info task) {

            long task_id = -1;

            _queue_lock.lock();

            _queues_task.push({
                _task_id_generated.gen_id(),
                Thread_Task_Description::HAS_TASK,
                task
            });

            task_id = _queues_task.front().task_id;

            _add_task_tracking(task_id, Thread_Task_State::WATIING);

            _is_has_task = true;

            _queue_lock.unlock();

            return task_id;
        }

        bool is_running() {
            return _is_running;
        }

        bool is_has_task() {
            return _is_has_task;
        }

        void start_running(int number_thread) {
            for (int i = 0;i < number_thread;i++) {
                Thread_Item_Extend* thread_item = new Thread_Item_Extend();
                std::thread t([this, thread_item] {
                    
                    _init_item_lock.lock();
                    init_item(thread_item);
                    _init_item_lock.unlock();

                    while (is_running()) {
                        if (is_has_task()) {
                            Task_Data task_data = get_task();
                            if (task_data.task_description != Thread_Task_Description::EMPTY) {
                                _add_task_tracking(task_data.task_id, Thread_Task_State::PROCESSING);
                                thread_item->do_task(task_data.task);
                                _add_task_tracking(task_data.task_id, Thread_Task_State::ENDING);
                            }
                        }
                    }
                });
                t.detach();

                _threads_items.push_back(thread_item);
            }
        }

        void stop_running() {
            _is_running = false;
        }

        void wait_to_task_end(const long& task_id) {
            Thread_Task_State task_state = _get_task_state(task_id);
            // Utility::Log::get()->log_info("task state", task_id, task_state);
            switch (task_state) {
                case Thread_Task_State::NONE: {
                    Utility::Log::get()->log_info("Waiting task with state none", task_id);
                    throw std::runtime_error("fail to wait task");
                    return;
                }
                case Thread_Task_State::ENDING: {
                    return;
                }
                default: {
                    while (_get_task_state(task_id) != Thread_Task_State::ENDING) {}
                    _task_tracking_lock.lock();
                    _processing_tasks.erase(task_id);
                    _task_tracking_lock.unlock();
                    break;
                }
            }
        }

        virtual void init_item(Thread_Item_Extend* thread_item) {

        }

        virtual void destroy_item(Thread_Item_Extend* thread_item) {

        }

        void destroy() {
            _is_running = false;
            for (auto& thread_item :_threads_items) {
                destroy_item(thread_item);
            }
        }

        Thread_Pool() {

        }

        virtual ~Thread_Pool() {
            for (auto& thread_item :_threads_items) {
                delete(thread_item);
            }
        }

    };

}