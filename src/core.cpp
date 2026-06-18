#include <core.h>

namespace Core {

	std::shared_ptr<ThreadPool> global_thread_pool = std::make_shared<ThreadPool>(std::thread::hardware_concurrency());

	std::shared_ptr<Scheduler> global_scheduler = std::make_shared<Scheduler>(global_thread_pool, 100);

} // namespace Core