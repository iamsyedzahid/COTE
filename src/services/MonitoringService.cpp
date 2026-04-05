#include "services/MonitoringService.hpp"
#include <sstream>
#include <iomanip>

MonitoringService::MonitoringService(MetricsService&           metrics,
                                     Logger&                   logger,
                                     std::chrono::milliseconds interval)
    : metrics_(metrics)
    , logger_(logger)
    , interval_(interval)
{}

MonitoringService::~MonitoringService() {
    stop();
}

void MonitoringService::start() {
    running_ = true;
    thread_  = std::thread(&MonitoringService::reportLoop, this);
}

void MonitoringService::stop() {
    if (!running_.exchange(false)) return;
    if (thread_.joinable()) thread_.join();
}

void MonitoringService::reportLoop() {
    while (running_.load()) {
        std::this_thread::sleep_for(interval_);
        if (!running_.load()) break;

        auto snap = metrics_.snapshot();

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1)
            << "METRICS | "
            << "Queued="    << snap.queued
            << " Running="  << snap.running
            << " Done="     << snap.completed
            << " Timeout="  << snap.timed_out
            << " Workers="  << snap.worker_count
            << " Thruput="  << snap.throughput_per_second << "/s"
            << " AvgLat="   << snap.average_latency_ms   << "ms";

        logger_.log(LogLevel::INFO, oss.str());
    }
}
