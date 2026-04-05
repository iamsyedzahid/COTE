#pragma once
#include "engine/WorkerPool.hpp"
#include "engine/TaskQueue.hpp"
#include "infrastructure/MetricsService.hpp"
#include <thread>
#include <atomic>
#include <memory>

class ScalingManager {
public:
    ScalingManager(std::shared_ptr<WorkerPool> pool,
                   std::shared_ptr<TaskQueue>  queue,
                   MetricsService&             metrics);
    ~ScalingManager();

    void start();
    void stop();

private:
    void monitorLoop();

    std::shared_ptr<WorkerPool> pool_;
    std::shared_ptr<TaskQueue>  queue_;
    MetricsService&             metrics_;

    std::atomic<bool> running_{false};
    std::thread       thread_;

    static constexpr size_t SCALE_UP_THRESHOLD   = 5;
    static constexpr size_t SCALE_DOWN_THRESHOLD = 1;
    static constexpr int    POLL_MS              = 500;
};
