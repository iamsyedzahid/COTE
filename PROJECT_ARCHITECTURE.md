# Project Architecture & Logic Flow: Cloud Task Orchestrator 🚀

This document explains the "How" and "Why" behind the project. Use this to prepare for your viva voce.

---

## 1. The Core Architecture (K8s Style)
This project follows a **Master-Worker** architecture, similar to how Kubernetes (K8s) manages cloud containers.

| Our Component | K8s Equivalent | Logic |
|---|---|---|
| **TaskService** | **API Server** | The entry point for all tasks. |
| **TaskQueue** | **etcd** | The thread-safe storage for all pending work. |
| **TaskScheduler** | **Control Plane** | Manages where tasks go and their priority. |
| **WorkerPool** | **Nodes / Kubelet** | The threads that actually "do" the work. |
| **ScalingManager** | **HPA (Autoscaler)** | Automatically adds or removes workers based on load. |

---

## 2. File-by-File Logic Breakdown

### `main.cpp` (The Entry Point)
*   **What it does**: Initializes all services (Logger, Metrics, Pool, Scaler).
*   **Logic**: It starts the "Client Threads" which simulate users sending tasks. It then enters the **Raylib UI Loop** where it listens for keys (SPACE, B, T) and updates the dashboard.
*   **Thought Process**: We separated the simulation logic from the UI logic so the dashboard stays smooth (30 FPS) even if the system is under heavy load.

### `WorkerPool.cpp` (The Muscle)
*   **Function: `workerLoop()`**: This is a infinite loop inside every thread. It uses a **Condition Variable** to "sleep" when the queue is empty and "wake up" only when a task is ready.
*   **Why?**: This is much better than "busy-waiting" (a loop that never stops), which would make your CPU usage hit 100% and heat up your laptop.

### `ScalingManager.cpp` (The Intelligence)
*   **Logic**: Every 1 second, it checks `queue.size()`. 
*   **Decision**: If `Queue Size > Workers * 2`, it calls `scaleUp()`. This is **Reactive Scaling**—responding to a burst of traffic exactly like a real cloud provider.

### `ThreadSafePriorityQueue.hpp` (The Safety)
*   **Mechanism**: Uses `std::mutex` and `std::lock_guard`.
*   **Problem it solves**: In multi-threading, two threads might try to "pop" the same task at the exact same nanosecond. This would cause a crash (Segmention Fault). The Mutex acts like a "Key" to a room—only one thread can enter the queue at a time.

---

## 3. Library Usage
1.  **Raylib**: Used for the graphical dashboard. We chose this over a standard console because it allows us to visualize complex things like **Throughput** and **Scaling** which are hard to see in text.
2.  **STL (Standard Template Library)**:
    *   `<thread>`: For parallel execution.
    *   `<atomic>`: For "lock-free" counters (like the job count) that are very fast.
    *   `<chrono>`: For high-precision timing (calculating latency in milliseconds).

---

## 4. Why this is "Advanced" for a Beginner
*   **Decoupled Design**: The UI doesn't know how the tasks run; it only looks at the `MetricsSnapshot`. This is a professional design pattern called **Separation of Concerns**.
*   **Resource Management**: We use a **Thread Pool**. Creating threads is expensive; reusing them is professional.
*   **Graceful Shutdown**: When you exit, the system tells all workers to finish their current job and then exit cleanly. This prevents memory leaks.

---

## 5. Potential Viva Questions
**Q: How do you handle a "Burst" of traffic?**
> **A:** The `ScalingManager` detects the queue buildup and scales the `WorkerPool` up to a maximum of 16 threads. Once the burst is cleared, it scales back down to save resources.

**Q: What happens if a task takes too long?**
> **A:** Every task has a `timeout`. If it exceeds this, the `TaskExecutor` sends a cancel signal to the task and marks it as **TIMED OUT** in the metrics.

**Q: Why use a Priority Queue instead of a normal one?**
> **A:** In the cloud, some jobs (like a user payment) are more important than others (like a background email). Priority scheduling ensures the most critical work is done first.
