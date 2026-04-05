#pragma once
#include "engine/TaskScheduler.hpp"
#include "infrastructure/Logger.hpp"
#include "models/Task.hpp"
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>

class TaskService {
public:
    TaskService(TaskScheduler& scheduler, Logger& logger);

    uint64_t submit(int                       priority,
                    std::chrono::milliseconds timeout,
                    std::function<void()>     work);

    size_t pendingCount() const;

private:
    TaskScheduler&    scheduler_;
    Logger&           logger_;
    std::atomic<uint64_t> next_id_{1};
};
