#pragma once
#ifndef SCHEDULER_H
#define SCHEDULER_H	


#include <vector>
#include <thread>
#include <functional>
#include <mutex>
#include <chrono>
#include <string>
#include <condition_variable>
#include <future>
#include <iostream>
#include <log.h>

#ifdef THREAD_POOL_H
#include <ThreadPool.h>
#endif // THREAD_POOL_H

constexpr auto TIME_NULL = long(-1);

class Scheduler {

	using Task_Function = std::function<void(long long)>;

	enum Schedule_Type {
		REPEAT_FOREVER,
		ONCE_ONLY
	};

	enum Excute_State {
		IDLE,
		EXCUTING,
		PAUSE
	};

	struct Pause_Task_Info {
		std::string task_name;
		bool pause_or_unpause;
	};

	struct Scheduled_Task {
		std::string task_name;
		Task_Function task;
		long long start_time = 0;
		long long required_time = 0; // in milliseconds
		Schedule_Type type = REPEAT_FOREVER;
		Excute_State excute_state = Excute_State::IDLE;
#ifdef THREAD_POOL_H
		std::future<void> excute_finish;
#endif //THREAD_POOL_H
	};

	private:

#ifdef THREAD_POOL_H
		std::shared_ptr<ThreadPool> _global_thread_pool = nullptr;
#endif // THREAD_POOL_H

		std::mutex tasks_mutex;

		std::vector<Scheduled_Task> tasks;

		std::thread looper_thread;

		std::condition_variable condition_variable;

		std::mutex pause_queue_mutex;

		std::vector<Pause_Task_Info> tasks_need_pause;

		bool is_stop = false;

		int min_loop_time_micrs = 1;

		long long _get_current_time_ms() {
			using namespace std::chrono;
			return static_cast<long long>(duration_cast<milliseconds>(
				system_clock::now().time_since_epoch()
			).count());
		}

		void do_task(Scheduled_Task& task_data, long long start_time) {
			task_data.excute_state = Excute_State::EXCUTING;
#ifdef THREAD_POOL_H
			if (_global_thread_pool != nullptr) {
				task_data.excute_finish = _global_thread_pool->enqueue([this, &task_data](long long start_time) {
					long long current_time = _get_current_time_ms();
					task_data.task(current_time - start_time);
					task_data.start_time = _get_current_time_ms();
					if (task_data.excute_state != Excute_State::PAUSE) {
						task_data.excute_state = Excute_State::IDLE;
					}
				}, start_time);
			}
			else {
				long long current_time = _get_current_time_ms();
				task_data.task(current_time - start_time);
				task_data.start_time = _get_current_time_ms();
				if (task_data.excute_state != Excute_State::PAUSE) {
					task_data.excute_state = Excute_State::IDLE;
				}
			}
#else	
			// When no thread pool is used, execute synchronously using the provided start_time
			long long current_time = _get_current_time_ms();
			task_data.task(current_time - start_time);
			task_data.start_time = _get_current_time_ms();
			if (task_data.excute_state != Excute_State::PAUSE) {
				task_data.excute_state = Excute_State::IDLE;
			}
#endif // THREAD_POOL_H
		}

		void push_task(Task_Function task, std::string task_key = "", long long delay_ms = TIME_NULL, Schedule_Type scheduled_type = Schedule_Type::REPEAT_FOREVER) {
			{
				std::unique_lock<std::mutex> lock(tasks_mutex);
				long long current_time = _get_current_time_ms();
				tasks.push_back(Scheduled_Task{ task_key, task, current_time, delay_ms , scheduled_type });
			}
			condition_variable.notify_one();
		}

		void start() {
			looper_thread = std::thread([this]() {
				for (;;) {

					std::unique_lock<std::mutex> lock_task(tasks_mutex);
					condition_variable.wait(lock_task, [this]() {
						return this->is_stop || (!this->tasks.empty() && !this->is_all_tasks_pause());
					});

					if (is_stop) {
						return;
					}

					{
						std::unique_lock<std::mutex> lock(pause_queue_mutex);
						for (int i = 0;i < tasks_need_pause.size();i++) {
							const auto& pause_info = tasks_need_pause[i];
							bool is_finish_pause = false;
							bool found_task = false;
							for (int i = 0;i < tasks.size();i++) {
								if (tasks[i].task_name == pause_info.task_name) {
									if (pause_info.pause_or_unpause && tasks[i].excute_state == Excute_State::IDLE) {
										tasks[i].excute_state = Excute_State::PAUSE;
										is_finish_pause = true;
									}
									if (!pause_info.pause_or_unpause && tasks[i].excute_state == Excute_State::PAUSE) {
										tasks[i].excute_state = Excute_State::IDLE;
										is_finish_pause = true;
									}
									found_task = true;
								}
							}
							if (is_finish_pause || !found_task) {
								tasks_need_pause[i] = tasks_need_pause[tasks_need_pause.size() - 1];
								tasks_need_pause.pop_back();
								i--;
							}
						}
					}
					Log::log_info("looper_thread 4");
					for (int i = 0; i < tasks.size(); ++i) {
						auto& task_data = tasks[i];
						/*
							if task is excuting or pause we wait 
							till it end and do at another loop
						*/
						if (task_data.excute_state == Excute_State::EXCUTING || task_data.excute_state == Excute_State::PAUSE) {
							continue;
						}
						long long current_time = _get_current_time_ms();

						bool is_task_done = false;
						if (task_data.required_time == TIME_NULL) {
							do_task(task_data, task_data.start_time);
							is_task_done = true;
						}
						else {
							long long elapsed_time = current_time - task_data.start_time;
							if (elapsed_time >= task_data.required_time) {
								do_task(task_data, task_data.start_time);
								is_task_done = true;
							}
						}

						/*
							if the task is once only and is done, remove it from the list
						*/
						if (is_task_done && task_data.type == Schedule_Type::ONCE_ONLY) {
							tasks.erase(tasks.begin() + i);
							--i;
						}
					}

					std::this_thread::sleep_for(std::chrono::microseconds(min_loop_time_micrs));
				}
				});
		}

		void stop() {
			is_stop = true;
			condition_variable.notify_one();
			if (looper_thread.joinable()) {
				looper_thread.join();
			}
		}

		bool is_all_tasks_pause() {
			for (const auto& task : tasks) {
				if (task.excute_state != Excute_State::PAUSE) {
					return false;
				}
			}
			return tasks_need_pause.size() <= 0;
		}

	public:

#ifdef THREAD_POOL_H
		Scheduler(std::shared_ptr<ThreadPool> global_thread_pool = nullptr, int min_loop_time_micrs = 1000)
			: _global_thread_pool(global_thread_pool), min_loop_time_micrs(min_loop_time_micrs){
			start();
		}
#else
		Scheduler(int min_loop_time_micrs = 1000)
			: min_loop_time_micrs(min_loop_time_micrs) {
			start();
		}
#endif // THREAD_POOL_H
		~Scheduler() {
			stop();
		}

		bool is_contain_task(const std::string& task_name) {
			for (const auto& task_data : tasks) {
				if (task_data.task_name == task_name) {
					return true;
				}
			}
			return false;
		}

		void schedule(Task_Function task, std::string task_name = "", long long delay_ms = TIME_NULL) {
			push_task(std::move(task), task_name, delay_ms, Schedule_Type::REPEAT_FOREVER);
		}

		void schedule_once(Task_Function task, std::string task_name = "",long long delay_ms = TIME_NULL) {
			push_task(std::move(task), task_name, delay_ms, Schedule_Type::ONCE_ONLY);
		}

		void remove_task_by_name(const std::string& task_name) {
			std::unique_lock<std::mutex> lock(tasks_mutex);
			for (int i = 0; i < tasks.size(); ++i) {
				if (tasks[i].task_name == task_name) {
#ifdef THREAD_POOL_H
					/*
						In case use core thread pool we need wait the task is excuted finish
						and erease it!
					*/
					if (tasks[i].excute_state == Excute_State::EXCUTING) {
						tasks[i].excute_finish.get();
					}
#endif
					tasks.erase(tasks.begin() + i);
					--i;
				}
			}
		}

		void pause_scheduler_task(const std::string& task_name) {
			std::unique_lock<std::mutex> lock(pause_queue_mutex);
			tasks_need_pause.push_back(Pause_Task_Info{
				task_name,  
				true
			});
		}

		void unpause_scheduler_task(const std::string& task_name) {
			std::unique_lock<std::mutex> lock(pause_queue_mutex);
			tasks_need_pause.push_back(Pause_Task_Info{
				task_name,
				false
			});
			condition_variable.notify_one();
		}
};

#endif