#pragma once
#include "infrastructure/ThreadSafePriorityQueue.hpp"
#include "models/Task.hpp"
#include <memory>

struct TaskPriorityComparator {
    bool operator()(const std::shared_ptr<Task>& a,
                    const std::shared_ptr<Task>& b) const {
        return a->priority < b->priority;
    }
};

using TaskQueue = ThreadSafePriorityQueue<std::shared_ptr<Task>, TaskPriorityComparator>;
