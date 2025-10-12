#pragma once
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <utility>
#include <exception>
#include <map>
#include <core/objects_id_generated.hpp>
#include <condition_variable>
#include <utility/log_utils.h>

namespace Core {

    /**
     * Contain all data using internal thread
     * and wapper the the task when it need
     * more job in do_task function
     */
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

        static_assert(
            std::is_base_of<Thread_Item<Task_Info>, Thread_Item_Extend>::value, 
            "Error: Thread_Pool<Thread_Item_Extend, Task_Info> requires Thread_Item_Extend<Task_Info> to inherit from Thread_Item."
        );

        /**
         * Wrapper struct is using
         * to add task id to Task_Info
         */
        struct Task_Data {
            long task_id;
            Task_Info task;
        };

        /**
         * Support struct to keep tracking
         * task when it is processing
         */
        struct Task_Tracking_Data {

            Thread_Task_State task_state;
            std::mutex* task_mutex = nullptr;
            std::condition_variable* task_condition = nullptr;
            
            void destroy() {
                delete(task_mutex);
                delete(task_condition);
            }
        };

        private:

        Longs_ID_Generated _task_id_generated;

        std::mutex _task_tracking_lock;

        std::mutex _queue_lock;

        std::mutex _init_item_lock;

        std::queue<Task_Data> _queues_task;

        std::condition_variable _queue_condition;

        std::vector<Thread_Item_Extend*> _threads_items;

        std::vector<std::thread> _workers;

        std::map<long, Task_Tracking_Data> _processing_tasks;

        bool _is_running = true;

        /**
         * Add state of task to task tracking data to keep track
         */
        void _add_task_tracking(const long& task_id, Thread_Task_State task_state) {
            std::unique_lock<std::mutex> lock(_task_tracking_lock);
            if (_processing_tasks.find(task_id) == _processing_tasks.end()) {
                _processing_tasks[task_id] = Task_Tracking_Data {
                    task_state,
                    new std::mutex(),
                    new std::condition_variable()
                };
            }
            else {
                _processing_tasks[task_id].task_state = task_state;
            }
        }

        /**
         * Query task tracking data by task id to keep track
         */
        Task_Tracking_Data _get_task_tracking_data(long task_id) {

            if (_processing_tasks.find(task_id) == _processing_tasks.end()) {
                return Task_Tracking_Data {
                    Thread_Task_State::NONE,
                    nullptr,
                    nullptr
                };
            }

            return _processing_tasks[task_id];
        }

        protected:

        /**
         * Init the item data is using internal task
         */
        virtual void init_item(Thread_Item_Extend* thread_item) {

        }

        /**
         * Destroy the item data is using internal task
         */
        virtual void destroy_item(Thread_Item_Extend* thread_item) {

        }

        public:

        bool is_running() {
            return _is_running;
        }

        /**
         * Thread pool will start running here with
         * number thread is using
         */
        void start_running(int number_thread) {
            
            // Reserve the number thread and data internal is needed
            _workers.reserve(number_thread);
            _threads_items.reserve(number_thread);

            for (int i = 0;i < number_thread;i++) {
                Thread_Item_Extend* thread_item = new Thread_Item_Extend();
                // Make worker to do task in multi thread
                _workers.emplace_back([this, thread_item] {
                    
                    // Init internal data will be using in thread
                    {
                        std::unique_lock<std::mutex> lock(_init_item_lock);
                        init_item(thread_item);
                    }
                    
                    for (;;) {
                        // Query a remain task data in queue to process
                        Task_Data task_data;
                        {
                            // Sleep the thread until queue has task
                            std::unique_lock<std::mutex> lock(_queue_lock);
                            this->_queue_condition.wait(lock, [this] () {
                                return !this->is_running() || !this->_queues_task.empty();
                            });

                            if (!this->is_running() && this->_queues_task.empty()) {
                                return;
                            }

                            task_data = std::move(_queues_task.front());
                            _queues_task.pop();
                        }

                        // Add tracking task to phase processing
                        _add_task_tracking(task_data.task_id, Thread_Task_State::PROCESSING);

                        // Do task here
                        thread_item->do_task(task_data.task);

                        // Add tracking task to phase end
                        _add_task_tracking(task_data.task_id, Thread_Task_State::ENDING);
                        
                        // Notify the thread block until task finish if has
                        Task_Tracking_Data task_tracking_data = this->_get_task_tracking_data(task_data.task_id);
                        {
                            switch (task_tracking_data.task_state) {
                                case Thread_Task_State::NONE: {

                                    break;
                                }
                                
                                default: {

                                    std::unique_lock<std::mutex> lock(*task_tracking_data.task_mutex);
                                    task_tracking_data.task_condition->notify_one();
                                    this->_processing_tasks.erase(task_data.task_id);
                                    break;
                                }
                            }
                        }
                    }
                });

                _threads_items.emplace_back(thread_item);
            }
        }

        /**
         * Task will be push here and return a task id
         * to tracking processing of task
         */
        long push_task(Task_Info task) {

            long task_id = -1;

            // Make a struct Task_Data to wrapper
            // Task_Info class with task id to tracking
            {
                std::unique_lock<std::mutex> lock(_queue_lock);
                _queues_task.push({
                    _task_id_generated.gen_id(),
                    task
                });

                task_id = _queues_task.front().task_id;
            }

            // Add state of task to tracking future
            _add_task_tracking(task_id, Thread_Task_State::WATIING);

            // Notify for some thread is sleep to do task
            _queue_condition.notify_one();

            // Return task id to tracking task
            return task_id;
        }

        void stop_running() {
            // Mark our thread pool as stop running
            {
                std::unique_lock<std::mutex> lock(_queue_lock);
                _is_running = false;
            }

            // Notify all thread to stop when finish all thier task
            _queue_condition.notify_all();
            for (std::thread& worker : _workers) {
                worker.join();
            }

            // Delete data of tracking task is using
            {
                std::unique_lock<std::mutex> lock(_task_tracking_lock);
                for (auto& [_, task_tracking_data] : _processing_tasks) {
                    task_tracking_data.destroy();
                }
            }
        }

        void wait_to_task_end(const long& task_id) {
            // Find task state in task tracking  data
            Task_Tracking_Data task_tracking_data = _get_task_tracking_data(task_id);

            // If none we throw exception to tracking invalid task
            switch (task_tracking_data.task_state) {
                case Thread_Task_State::NONE: {
                    Utility::Log::get()->log_info("Waiting task with state none", task_id);
                    throw std::runtime_error("fail to wait task");
                    return;
                }
                // If already ending we just return
                case Thread_Task_State::ENDING: {
                    return;
                }
                default: {
                    break;
                }
            }

            // In other state waiting and loading we need sleep current thread
            // and wait until it's processed finish
            std::unique_lock<std::mutex> lock(*task_tracking_data.task_mutex);
            task_tracking_data.task_condition->wait(lock, [this, task_id]() {
                Task_Tracking_Data task_tracking_data = _get_task_tracking_data(task_id);
                return !this->is_running() || task_tracking_data.task_state == Thread_Task_State::ENDING || task_tracking_data.task_state == Thread_Task_State::NONE;
            });
        }

        /**
         * Thread pool will stop running and clear
         * all data is using when call destroy
         */
        void destroy() {

            // do stop running phase
            stop_running();

            // clear all data item is using internal thread 
            for (auto& thread_item : _threads_items) {
                destroy_item(thread_item);
            }
        }

        Thread_Pool() {

        }

        virtual ~Thread_Pool() {
            // Destroy all pointer of data is using internal thread
            for (auto& thread_item :_threads_items) {
                delete(thread_item);
            }
        }

    };

}