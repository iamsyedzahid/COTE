#pragma once
#include "infrastructure/MetricsService.hpp"
#include "infrastructure/Logger.hpp"
#include <thread>
#include <atomic>
#include <chrono>

class MonitoringService {
public:
    MonitoringService(MetricsService& metrics,
                      Logger&         logger,
                      std::chrono::milliseconds interval);
    ~MonitoringService();

    void start();
    void stop();

private:
    void reportLoop();

    MetricsService&           metrics_;
    Logger&                   logger_;
    std::chrono::milliseconds interval_;
    std::atomic<bool>         running_{false};
    std::thread               thread_;
};
