#pragma once

#include <ThreadPool.h>
#include <Scheduler.h>

namespace Core {

	extern std::shared_ptr<ThreadPool> global_thread_pool;

	extern std::shared_ptr<Scheduler> global_scheduler;

} // namespace Core