#include "services/TaskService.hpp"
#include <sstream>

TaskService::TaskService(TaskScheduler& scheduler, Logger& logger)
    : scheduler_(scheduler)
    , logger_(logger)
{}

uint64_t TaskService::submit(int                       priority,
                             std::chrono::milliseconds timeout,
                             std::function<void()>     work) {
    uint64_t id      = next_id_++;
    auto     cancel  = std::make_shared<std::atomic<bool>>(false);
    auto     task    = std::make_shared<Task>(id, priority, timeout, std::move(work), cancel);

    std::ostringstream msg;
    msg << "Task #" << id << " submitted  priority=" << priority
        << "  timeout=" << timeout.count() << "ms";
    logger_.log(LogLevel::INFO, msg.str());

    scheduler_.submit(std::move(task));
    return id;
}

size_t TaskService::pendingCount() const {
    return scheduler_.queueSize();
}
