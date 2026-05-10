#include "models/Task.hpp"

Task::Task(uint64_t id,
           int priority,
           std::chrono::milliseconds timeout,
           std::function<void()> work,
           int duration_ms,
           std::shared_ptr<std::atomic<bool>> cancel_flag)
    : id(id)
    , priority(priority)
    , timeout(timeout)
    , work(std::move(work))
    , duration_ms(duration_ms)
    , cancel_flag(std::move(cancel_flag))
    , state(TaskState::Queued)
    , submit_time(std::chrono::steady_clock::now())
{}
