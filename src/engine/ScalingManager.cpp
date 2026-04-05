#include "engine/ScalingManager.hpp"
#include <chrono>

ScalingManager::ScalingManager(std::shared_ptr<WorkerPool> pool,
                               std::shared_ptr<TaskQueue>  queue,
                               MetricsService&             metrics)
    : pool_(std::move(pool))
    , queue_(std::move(queue))
    , metrics_(metrics)
{}

ScalingManager::~ScalingManager() {
    stop();
}

void ScalingManager::start() {
    running_ = true;
    thread_  = std::thread(&ScalingManager::monitorLoop, this);
}

void ScalingManager::stop() {
    if (!running_.exchange(false)) return;
    if (thread_.joinable()) thread_.join();
}

void ScalingManager::monitorLoop() {
    while (running_.load()) {
        size_t depth   = queue_->size();
        int    current = pool_->targetCount();

        if (depth > SCALE_UP_THRESHOLD) {
            int needed = static_cast<int>(depth / SCALE_UP_THRESHOLD);
            pool_->scaleUp(needed);
        } else if (depth < SCALE_DOWN_THRESHOLD && current > 1) {
            pool_->scaleDown(1);
        }

        metrics_.setQueueSize(static_cast<uint64_t>(depth));
        std::this_thread::sleep_for(std::chrono::milliseconds(POLL_MS));
    }
}
