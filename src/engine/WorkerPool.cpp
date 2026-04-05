#include "engine/WorkerPool.hpp"
#include <algorithm>

WorkerPool::WorkerPool(std::shared_ptr<TaskQueue> queue,
                       MetricsService&            metrics,
                       int                        initial_workers,
                       int                        min_workers,
                       int                        max_workers)
    : queue_(std::move(queue))
    , metrics_(metrics)
    , min_workers_(min_workers)
    , max_workers_(max_workers)
{
    int clamped = std::clamp(initial_workers, min_workers_, max_workers_);
    for (int i = 0; i < clamped; ++i) spawnWorker();
}

WorkerPool::~WorkerPool() {
    shutdown();
}

void WorkerPool::spawnWorker() {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    ++target_count_;
    workers_.emplace_back(&WorkerPool::workerLoop, this);
}

void WorkerPool::scaleUp(int count) {
    int current = target_count_.load();
    int target  = std::min(current + count, max_workers_);
    for (int i = current; i < target; ++i) spawnWorker();
}

void WorkerPool::scaleDown(int count) {
    int current = target_count_.load();
    int target  = std::max(current - count, min_workers_);
    target_count_.store(target);
}

void WorkerPool::scaleTo(int target) {
    target = std::clamp(target, min_workers_, max_workers_);
    int current = target_count_.load();
    if (target > current) {
        for (int i = current; i < target; ++i) spawnWorker();
    } else {
        target_count_.store(target);
    }
}

int WorkerPool::targetCount() const {
    return target_count_.load();
}

int WorkerPool::activeCount() const {
    return active_count_.load();
}

void WorkerPool::shutdown() {
    if (!running_.exchange(false)) return;
    queue_->stop();
    std::lock_guard<std::mutex> lock(workers_mutex_);
    for (auto& t : workers_) {
        if (t.joinable()) t.join();
    }
    workers_.clear();
}

void WorkerPool::workerLoop() {
    ++active_count_;
    metrics_.setWorkerCount(active_count_.load());

    while (running_.load()) {
        if (active_count_.load() > target_count_.load()) break;

        std::shared_ptr<Task> task;
        if (queue_->wait_and_pop(task, std::chrono::milliseconds(100))) {
            executor_.execute(task, metrics_);
        }
    }

    --active_count_;
    metrics_.setWorkerCount(active_count_.load());
}
