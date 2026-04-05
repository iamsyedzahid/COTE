#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct TaskEvent {
    uint64_t    id;
    int         priority;
    std::string state;   // "QUEUED" | "RUNNING" | "DONE" | "TIMEOUT"
};

struct MetricsSnapshot {
    uint64_t queued;
    uint64_t running;
    uint64_t completed;
    uint64_t timed_out;
    uint64_t total_submitted;
    double   throughput_per_second;
    double   average_latency_ms;
    int      worker_count;

    std::vector<TaskEvent> recent_events;  // last 10 task state changes
};
