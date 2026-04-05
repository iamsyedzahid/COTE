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
static constexpr int CLIENT_THREADS  = 5;
static constexpr int TASKS_PER_CLIENT= 40;

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

    std::atomic<bool>   clients_done{false};
    std::vector<std::thread> client_threads;

    for (int c = 0; c < CLIENT_THREADS; ++c) {
        client_threads.emplace_back([&, c] {
            std::mt19937                                rng(42 + c);
            std::uniform_int_distribution<int>          priority_dist(1, 10);
            std::uniform_int_distribution<int>          work_ms_dist(1500, 5000);   // tasks take 1.5–5s
            std::uniform_int_distribution<int>          timeout_ms_dist(6000, 12000); // generous timeout 6–12s
            std::uniform_int_distribution<int>          gap_ms_dist(300, 800);      // submit 1 task/~0.5s per client

            for (int i = 0; i < TASKS_PER_CLIENT; ++i) {
                int  prio       = priority_dist(rng);
                int  work_ms    = work_ms_dist(rng);
                int  timeout_ms = timeout_ms_dist(rng);

                std::ostringstream tag;
                tag << "client-" << c << "-job-" << i;
                std::string label = tag.str();

                auto cancel = std::make_shared<std::atomic<bool>>(false);

                task_service.submit(prio,
                    std::chrono::milliseconds(timeout_ms),
                    [work_ms, label, &execution_slots, cancel] {
                        execution_slots.acquire();
                        auto deadline = std::chrono::steady_clock::now()
                                        + std::chrono::milliseconds(work_ms);
                        while (std::chrono::steady_clock::now() < deadline) {
                            if (cancel->load()) break;
                            std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        }
                        execution_slots.release();
                    }
                );

                std::this_thread::sleep_for(std::chrono::milliseconds(gap_ms_dist(rng)));
            }
        });
    }

    Dashboard dashboard(metrics, 1100, 600);

    while (!dashboard.shouldClose()) {
        dashboard.update();
        dashboard.render();
    }

    for (auto& t : client_threads)
        if (t.joinable()) t.join();

    clients_done = true;
    monitor.stop();
    scaler.stop();
    pool->shutdown();

    logger.log(LogLevel::INFO, "Graceful shutdown complete");
    return 0;
}
