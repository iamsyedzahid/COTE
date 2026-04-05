#include "infrastructure/MetricsService.hpp"
#include <numeric>
#include <algorithm>

void MetricsService::recordSubmission() {
    ++total_submitted_;
}

void MetricsService::recordCompletion(double latency_ms) {
    ++total_completed_;
    std::lock_guard<std::mutex> lock(data_mutex_);
    recent_latencies_.push_back(latency_ms);
    if (recent_latencies_.size() > MAX_LATENCY_SAMPLES)
        recent_latencies_.pop_front();
    completion_times_.push_back(std::chrono::steady_clock::now());
}

void MetricsService::recordTimeout() {
    ++total_timed_out_;
}

void MetricsService::adjustRunning(int delta) {
    running_count_ += delta;
}

void MetricsService::setQueueSize(uint64_t size) {
    queue_size_ = size;
}

void MetricsService::setWorkerCount(int count) {
    worker_count_ = count;
}

void MetricsService::recordEvent(uint64_t id, int priority, const std::string& state) {
    std::lock_guard<std::mutex> lock(data_mutex_);
    event_log_.push_back({id, priority, state});
    if (event_log_.size() > MAX_EVENT_LOG)
        event_log_.pop_front();
}

MetricsSnapshot MetricsService::snapshot() const {
    MetricsSnapshot snap;
    snap.total_submitted = total_submitted_.load();
    snap.completed       = total_completed_.load();
    snap.timed_out       = total_timed_out_.load();
    snap.running         = static_cast<uint64_t>(std::max(0, running_count_.load()));
    snap.queued          = queue_size_.load();
    snap.worker_count    = worker_count_.load();

    std::lock_guard<std::mutex> lock(data_mutex_);

    snap.recent_events = {event_log_.begin(), event_log_.end()};

    snap.average_latency_ms = recent_latencies_.empty()
        ? 0.0
        : std::accumulate(recent_latencies_.begin(), recent_latencies_.end(), 0.0)
          / static_cast<double>(recent_latencies_.size());

    auto now          = std::chrono::steady_clock::now();
    auto window_start = now - std::chrono::seconds(THROUGHPUT_WINDOW_SEC);

    while (!completion_times_.empty() && completion_times_.front() < window_start)
        completion_times_.pop_front();

    snap.throughput_per_second = static_cast<double>(completion_times_.size());

    return snap;
}
