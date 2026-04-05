#pragma once
#include "engine/TaskQueue.hpp"
#include "engine/TaskExecutor.hpp"
#include "infrastructure/MetricsService.hpp"
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>

class WorkerPool {
public:
    WorkerPool(std::shared_ptr<TaskQueue> queue,
               MetricsService&           metrics,
               int                       initial_workers,
               int                       min_workers,
               int                       max_workers);
    ~WorkerPool();

    void scaleUp(int count = 1);
    void scaleDown(int count = 1);
    void scaleTo(int target);
    int  targetCount() const;
    int  activeCount() const;
    void shutdown();

private:
    void workerLoop();
    void spawnWorker();

    std::shared_ptr<TaskQueue> queue_;
    MetricsService&            metrics_;
    TaskExecutor               executor_;

    int min_workers_;
    int max_workers_;

    std::atomic<int>  target_count_{0};
    std::atomic<int>  active_count_{0};
    std::atomic<bool> running_{true};

    mutable std::mutex       workers_mutex_;
    std::vector<std::thread> workers_;
};
