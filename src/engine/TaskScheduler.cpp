#include "engine/TaskScheduler.hpp"

TaskScheduler::TaskScheduler(std::shared_ptr<TaskQueue>  queue,
                             std::shared_ptr<WorkerPool> worker_pool,
                             MetricsService&             metrics)
    : queue_(std::move(queue))
    , worker_pool_(std::move(worker_pool))
    , metrics_(metrics)
{}

void TaskScheduler::submit(std::shared_ptr<Task> task) {
    metrics_.recordSubmission();
    metrics_.recordEvent(task->id, task->priority, "QUEUED");
    queue_->push(std::move(task));
    metrics_.setQueueSize(queue_->size());
}

size_t TaskScheduler::queueSize() const {
    return queue_->size();
}
