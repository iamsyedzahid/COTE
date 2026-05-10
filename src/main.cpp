#include "infrastructure/Logger.hpp"
#include "infrastructure/MetricsService.hpp"
#include "infrastructure/Semaphore.hpp"
#include "engine/TaskQueue.hpp"
#include "engine/TaskScheduler.hpp"
#include "engine/WorkerPool.hpp"
#include "engine/ScalingManager.hpp"
#include "services/TaskService.hpp"
#include "services/MonitoringService.hpp"
#include "presentation/Dashboard.hpp"

#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <random>
#include <functional>
#include <memory>
#include <sstream>

static constexpr int INITIAL_WORKERS = 4;
static constexpr int MIN_WORKERS     = 2;
static constexpr int MAX_WORKERS     = 16;
static constexpr int MAX_SLOTS       = 8;

int main() {
    Logger          logger;
    MetricsService  metrics;
    Semaphore       execution_slots(MAX_SLOTS);

    auto queue   = std::make_shared<TaskQueue>();
    auto pool    = std::make_shared<WorkerPool>(queue, metrics,
                                                INITIAL_WORKERS,
                                                MIN_WORKERS,
                                                MAX_WORKERS);
    TaskScheduler    scheduler(queue, pool, metrics);
    ScalingManager   scaler(pool, queue, metrics);
    TaskService      task_service(scheduler, logger);
    MonitoringService monitor(metrics, logger, std::chrono::milliseconds(1000));

    scaler.start();
    monitor.start();

    logger.log(LogLevel::INFO, "System started — worker pool online");

    // Client simulation threads
    std::vector<std::thread> client_threads;
    const int CLIENT_COUNT     = 2;
    const int TASKS_PER_CLIENT = 100;

    for (int c = 0; c < CLIENT_COUNT; ++c) {
        client_threads.emplace_back([&, c] {
            std::mt19937                                rng(42 + c);
            std::uniform_int_distribution<int>          priority_dist(1, 10);
            std::uniform_int_distribution<int>          work_ms_dist(5000, 15000);    // Background: 5–15s  = 5000 ms and 15000 ms
            std::uniform_int_distribution<int>          timeout_ms_dist(30000, 45000); 
            std::uniform_int_distribution<int>          gap_ms_dist(10000, 20000);    // Every 10–20s

            for (int i = 0; i < TASKS_PER_CLIENT; ++i) {
                // Wait if simulation is paused
                while (metrics.snapshot().is_paused) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }

                int  prio       = priority_dist(rng);
                int  work_ms    = work_ms_dist(rng);
                auto timeout_ms = std::chrono::milliseconds(timeout_ms_dist(rng));

                task_service.submit(prio, timeout_ms, [work_ms] {
                    std::this_thread::sleep_for(std::chrono::milliseconds(work_ms));
                }, work_ms);

                std::this_thread::sleep_for(std::chrono::milliseconds(gap_ms_dist(rng)));
            }
        });
    }

    Dashboard dashboard(metrics, 1100, 600);

    while (!dashboard.shouldClose()) {
        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

        // SPACE: Toggle Pause
        if (IsKeyPressed(KEY_SPACE)) {
            bool current = metrics.snapshot().is_paused;
            pool->setPaused(!current);
            metrics.setPaused(!current);
            logger.log(LogLevel::INFO, !current ? "SIMULATION PAUSED" : "SIMULATION RESUMED");
        }

        // B: Manual Burst (25 tasks)
        if (IsKeyPressed(KEY_B)) {
            logger.log(LogLevel::INFO, "MANUAL BURST: Injecting 25 jobs (15s each)!");
            for (int i = 0; i < 25; ++i) {
                task_service.submit(10, std::chrono::milliseconds(45000), [] {
                    std::this_thread::sleep_for(std::chrono::milliseconds(15000));
                }, 15000);
            }
        }

        // T: Single Task
        if (IsKeyPressed(KEY_T)) {
            logger.log(LogLevel::INFO, "MANUAL TASK: Injecting single high-priority job!");
            task_service.submit(10, std::chrono::milliseconds(30000), [] {
                std::this_thread::sleep_for(std::chrono::milliseconds(5000));
            }, 5000);
        }

        dashboard.update();
        dashboard.render();
    }

    for (auto& t : client_threads)
        if (t.joinable()) t.join();

    monitor.stop();
    scaler.stop();
    pool->shutdown();

    logger.log(LogLevel::INFO, "Graceful shutdown complete");
    return 0;
}
