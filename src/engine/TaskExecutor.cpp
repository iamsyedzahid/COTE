#include "engine/TaskExecutor.hpp"
#include <future>
#include <chrono>

void TaskExecutor::execute(std::shared_ptr<Task> task, MetricsService& metrics) {
    task->state.store(TaskState::Running);
    task->start_time = std::chrono::steady_clock::now();
    metrics.adjustRunning(1);
    metrics.recordEvent(task->id, task->priority, "RUNNING");

    auto fut = std::async(std::launch::async, [task] { task->work(); });

    if (fut.wait_for(task->timeout) == std::future_status::timeout) {
        task->cancel_flag->store(true);
        fut.wait();
        task->state.store(TaskState::TimedOut);
        metrics.adjustRunning(-1);
        metrics.recordTimeout();
        metrics.recordEvent(task->id, task->priority, "TIMEOUT");
    } else {
        task->state.store(TaskState::Completed);
        metrics.adjustRunning(-1);
        auto latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - task->submit_time).count();
        metrics.recordCompletion(static_cast<double>(latency_ms));
        metrics.recordEvent(task->id, task->priority, "DONE");
    }

    task->end_time = std::chrono::steady_clock::now();
}
