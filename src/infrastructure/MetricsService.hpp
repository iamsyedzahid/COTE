#pragma once
#include "models/MetricsSnapshot.hpp"
#include <atomic>
#include <deque>
#include <mutex>
#include <chrono>
#include <cstdint>

class MetricsService {
public:
    void recordSubmission();
    void recordCompletion(double latency_ms);
    void recordTimeout();
    void adjustRunning(int delta);
    void setQueueSize(uint64_t size);
    void setWorkerCount(int count);
    void recordEvent(uint64_t id, int priority, const std::string& state);

    MetricsSnapshot snapshot() const;

private:
    std::atomic<uint64_t> total_submitted_{0};
    std::atomic<uint64_t> total_completed_{0};
    std::atomic<uint64_t> total_timed_out_{0};
    std::atomic<int>      running_count_{0};
    std::atomic<uint64_t> queue_size_{0};
    std::atomic<int>      worker_count_{0};

    mutable std::mutex    data_mutex_;
    mutable std::deque<double>                              recent_latencies_;
    mutable std::deque<std::chrono::steady_clock::time_point> completion_times_;
    std::deque<TaskEvent>                                   event_log_;

    static constexpr size_t MAX_LATENCY_SAMPLES  = 100;
    static constexpr int    THROUGHPUT_WINDOW_SEC = 1;
    static constexpr size_t MAX_EVENT_LOG        = 10;
};
