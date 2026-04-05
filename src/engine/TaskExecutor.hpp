#pragma once
#include "models/Task.hpp"
#include "infrastructure/MetricsService.hpp"
#include <memory>

class TaskExecutor {
public:
    void execute(std::shared_ptr<Task> task, MetricsService& metrics);
};
