#pragma once

#include <Scheduler.h>
#include <ThreadPool.h>
#include <ecs.h>

namespace Core {

	extern std::shared_ptr<ThreadPool> global_thread_pool;

	extern std::shared_ptr<Scheduler> global_scheduler;

	// extern std::shared_ptr<Registry> global_registry;

} // namespace Core