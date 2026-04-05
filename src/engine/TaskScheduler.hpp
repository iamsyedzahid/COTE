#pragma once
#include "engine/TaskQueue.hpp"
#include "engine/WorkerPool.hpp"
#include "infrastructure/MetricsService.hpp"
#include <memory>

class TaskScheduler {
public:
    TaskScheduler(std::shared_ptr<TaskQueue>  queue,
                  std::shared_ptr<WorkerPool> worker_pool,
                  MetricsService&             metrics);

    void   submit(std::shared_ptr<Task> task);
    size_t queueSize() const;

private:
    std::shared_ptr<TaskQueue>  queue_;
    std::shared_ptr<WorkerPool> worker_pool_;
    MetricsService&             metrics_;
};
