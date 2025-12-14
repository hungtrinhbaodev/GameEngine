#pragma once
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <map>
#include <iostream>

class ThreadPool {
public:
    ThreadPool(size_t);

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result<F, Args...>::type>;

    template<class F, class ...Args>
    auto loop_all_threads(F&& f, Args&&... args)
        -> std::map<std::thread::id, std::future<typename std::invoke_result<F, Args...>::type>>;

    const std::vector<std::thread::id>& get_thread_ids();

    ~ThreadPool();
private:

    // personal thread task
    std::map<std::thread::id, std::queue<std::function<void()>>> personal_tasks;

    // need to keep track of threads so we can join them
    std::vector< std::thread > workers;

    // threads ids
    std::vector < std::thread::id > thread_ids;

    // the task queue
    std::queue< std::function<void()> > tasks;

    // get personal tasks is done in thread id
    std::queue<std::function<void()>>& get_personal_tasks(std::thread::id thread_id);
    
    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};
 
// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t threads)
    :   stop(false)
{
    thread_ids.resize(threads);

    for(size_t i = 0;i<threads;++i) {

        auto init_func = [this, i]() {
			std::thread::id thread_id = std::this_thread::get_id();
            thread_ids[i] = thread_id;
            personal_tasks[thread_id] = {};
            return 0;
        };

        auto init_task = std::make_shared<std::packaged_task<int()>> (std::forward<std::function<int()>>(init_func));
        std::future<int> init_res = init_task->get_future();

        workers.emplace_back(
            [this, init_task]
            {
                (*init_task)();

				std::thread::id thread_id = std::this_thread::get_id();

                for (;;)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock,
                            [this, thread_id] { 
                                return this->stop || !this->tasks.empty() || !this->get_personal_tasks(thread_id).empty();
                            });
                        auto& personal_tasks = this->get_personal_tasks(thread_id);
                        if (this->stop && this->tasks.empty() && personal_tasks.empty())
                            return;
                        if (!personal_tasks.empty()) {
                            task = std::move(personal_tasks.front());
                            personal_tasks.pop();
                        }
                        else if (!this->tasks.empty()){
                            task = std::move(this->tasks.front());
                            this->tasks.pop();
                        }
                        else {
                            continue;
                        }
                    }

                    task();
                }
            }
        );

        init_res.get();
    }
}

// add new work item to the pool
template<class F, class... Args>
inline auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type>
{
    using return_type = typename std::invoke_result<F, Args...>::type;
    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // don't allow enqueueing after stopping the pool
        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace([task](){ (*task)(); });
    }
    condition.notify_one();
    return res;
}

inline const std::vector<std::thread::id>& ThreadPool::get_thread_ids() {
    return thread_ids;
}

inline std::queue<std::function<void()>>& ThreadPool::get_personal_tasks(std::thread::id thread_id) {
    return personal_tasks[thread_id];
}

// add work that all thread need to do
template<class F, class... Args>
inline auto ThreadPool::loop_all_threads(F&& f, Args&&... args)
    ->std::map<std::thread::id, std::future<typename std::invoke_result<F, Args...>::type>> {

    using result_type = typename std::invoke_result<F, Args...>::type;
    std::map<std::thread::id, std::future<result_type>> result_res;

    for (auto& thread_id : thread_ids) {

        auto& personal_tasks = get_personal_tasks(thread_id); 

        auto task = std::make_shared< std::packaged_task<result_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        result_res[thread_id] = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            if (stop) {
				std::cout << "Fail to loop all threads, thread pool is stopped." << std::endl;
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }

            personal_tasks.emplace([task]() {
                (*task)();
            });
        }
    }

    condition.notify_all();

    return result_res;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for(std::thread &worker: workers)
        worker.join();
}

#endif
