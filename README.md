# Cloud Orchestration Task Engine

> A production-grade multithreaded task scheduler simulation in C++17 with a real-time Raylib GUI dashboard — inspired by Kubernetes and cloud container orchestration systems.

---

## Features

### Mandatory
| Feature | Implementation |
|---|---|
| Shared task queue with mutex | `ThreadSafePriorityQueue` with `std::mutex` + `std::condition_variable` |
| Fixed-size worker thread pool | `WorkerPool` with configurable initial/min/max thread counts |
| Semaphore-controlled execution slots | Custom `Semaphore` class limits concurrent active tasks |
| Task completion reporting | `MetricsService` tracks all state transitions atomically |

### Additional (5/5)
| Feature | Implementation |
|---|---|
| Priority-based scheduling | `TaskPriorityComparator` in `TaskQueue` — higher priority executes first |
| Task timeout handling | `TaskExecutor` uses `std::async` + `wait_for`; sets `cancel_flag` on timeout |
| Dynamic worker scaling | `ScalingManager` polls queue depth every 500ms and adjusts pool size |
| Logging and monitoring thread | Dedicated `Logger` thread + `MonitoringService` reports stats every 1s |
| Performance statistics | `MetricsService` — sliding-window throughput, rolling-average latency |

### GUI (20%)
- Real-time Raylib dashboard at 30 FPS
- Task count cards: Queued / Running / Completed / Timed Out
- Worker thread visualizer bar
- Three live scrolling graphs: Throughput, Avg Latency, Queue Depth
- Performance stats panel with success rate

---

## Architecture

```
Presentation Layer  (Dashboard — Raylib GUI)
        ↓
Application Layer   (TaskService, MonitoringService)
        ↓
Core Engine Layer   (TaskScheduler, WorkerPool, ScalingManager, TaskExecutor)
        ↓
Infrastructure Layer (ThreadSafePriorityQueue, Semaphore, Logger, MetricsService)
        ↓
Models Layer        (Task, TaskState, MetricsSnapshot)
```

---

## Project Structure

```
CloudTaskOrchestrator/
├── CMakeLists.txt
├── build.sh          (Linux/macOS)
├── build.bat         (Windows)
├── .gitignore
├── README.md
└── src/
    ├── main.cpp
    ├── models/
    │   ├── Task.hpp / Task.cpp
    │   ├── TaskState.hpp
    │   └── MetricsSnapshot.hpp
    ├── infrastructure/
    │   ├── ThreadSafePriorityQueue.hpp
    │   ├── Semaphore.hpp / Semaphore.cpp
    │   ├── Logger.hpp / Logger.cpp
    │   └── MetricsService.hpp / MetricsService.cpp
    ├── engine/
    │   ├── TaskQueue.hpp
    │   ├── TaskExecutor.hpp / TaskExecutor.cpp
    │   ├── WorkerPool.hpp / WorkerPool.cpp
    │   ├── TaskScheduler.hpp / TaskScheduler.cpp
    │   └── ScalingManager.hpp / ScalingManager.cpp
    ├── services/
    │   ├── TaskService.hpp / TaskService.cpp
    │   └── MonitoringService.hpp / MonitoringService.cpp
    └── presentation/
        ├── GraphBuffer.hpp / GraphBuffer.cpp
        └── Dashboard.hpp / Dashboard.cpp
```

---

## Build Instructions

### Prerequisites
- CMake ≥ 3.16
- C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+)
- Git (for FetchContent to download Raylib 5.0 automatically)
- Linux: `libgl-dev`, `libx11-dev`, `libxi-dev` (usually pre-installed)

### Linux / macOS
```bash
git clone <your-repo-url>
cd CloudTaskOrchestrator
chmod +x build.sh
./build.sh
./build/orchestrator
```

### Windows (PowerShell)
```powershell
git clone <your-repo-url>
cd CloudTaskOrchestrator

.\build.bat
.\build\Release\orchestrator.exe
```

### Manual CMake
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/orchestrator
```
#### **OR**
**```bash**

# 1. Rebuild (fast since it's just a few files)
cmake --build build --parallel

# 2. Run
./build/orchestrator.exe

---

## Demo Scenario (For Viva)

1. Launch — observe workers starting at 4
2. Client threads flood the queue — watch queue depth spike
3. `ScalingManager` adds workers — worker bar expands live
4. High-priority tasks execute before low-priority ones
5. Tasks with short timeout get cancelled — Timed Out card increments
6. Load drops — workers scale back down
7. Monitor console output: stats printed every second

---

## Design Decisions (Viva Q&A)

**Why `condition_variable` instead of spin-waiting?**
Avoids burning CPU cycles — threads block at OS level and wake only when work is available. This is identical to how real thread pools work in Linux kernel task queues.

**Why `std::atomic` on `TaskState`?**
Multiple threads (worker + timeout monitor) may read/write state simultaneously. Atomics give lock-free, sequentially consistent state transitions with no overhead.

**Why separate `Semaphore` on top of the thread pool?**
The pool controls *how many threads exist*. The semaphore controls *how many execute concurrently* — analogous to Kubernetes resource limits (`limits.cpu`) vs replica count.

**Why `priority_queue` with a custom comparator?**
Mirrors real schedulers: Linux CFS uses virtual runtime; Kubernetes uses `PriorityClass`. Our comparator ensures highest-priority tasks are always dequeued first, preventing starvation of critical jobs.

**Why `ScalingManager` as a separate thread?**
Separation of concerns — scaling policy is independent from execution policy. This mirrors the Kubernetes Horizontal Pod Autoscaler (HPA), a separate control loop.

---

## Group Members
- Member 1 — Student ID: 24K0015
- Member 2 — Student ID: 24K0013
