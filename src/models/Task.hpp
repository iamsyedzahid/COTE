#pragma once
#include "models/TaskState.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>

struct Task {
    uint64_t                                    id;
    int                                         priority;
    std::chrono::milliseconds                   timeout;
    std::function<void()>                       work;
    std::shared_ptr<std::atomic<bool>>          cancel_flag;
    std::atomic<TaskState>                      state;
    std::chrono::steady_clock::time_point       submit_time;
    std::chrono::steady_clock::time_point       start_time;
    std::chrono::steady_clock::time_point       end_time;

    Task(uint64_t id,
         int priority,
         std::chrono::milliseconds timeout,
         std::function<void()> work,
         std::shared_ptr<std::atomic<bool>> cancel_flag);

    Task(const Task&)            = delete;
    Task& operator=(const Task&) = delete;
};
