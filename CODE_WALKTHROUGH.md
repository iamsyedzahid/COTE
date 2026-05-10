# Line-by-Line Code Walkthrough 🕵️‍♂️

This guide zooms into the actual C++ code and explains the most critical lines.

---

## 1. `src/main.cpp`: The Simulation Controller

```cpp
// Line 49: Defining the simulation parameters
const int CLIENT_COUNT = 2; 
const int TASKS_PER_CLIENT = 100;
```
*   **Explanation**: We set up 2 "simulated users" who will each send 100 tasks.

```cpp
// Line 63: The Pause Check
while (metrics.snapshot().is_paused) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}
```
*   **Explanation**: This is the "Pause" logic. Before submitting a new task, the client thread checks if you pressed SPACE. If you did, it waits here until you resume.

```cpp
// Line 82: The Main UI Loop
while (!dashboard.shouldClose()) {
    if (IsKeyPressed(KEY_B)) { ... } // Listen for Burst key
    dashboard.update();              // Calculate new data points
    dashboard.render();              // Draw everything on screen
}
```
*   **Explanation**: This is the heartbeat of the app. It runs at 30 FPS, keeping the UI responsive and the graphs moving.

---

## 2. `src/engine/WorkerPool.cpp`: The Thread Management

```cpp
// Line 73: The Kubelet Loop
if (paused_.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    continue;
}
```
*   **Explanation**: If the simulation is paused, the worker stops looking for new tasks. It just "idles" until the pause is lifted.

```cpp
// Line 78: Thread-Safe Task Pickup
if (queue_->wait_and_pop(task, std::chrono::milliseconds(100))) {
    executor_.execute(task, metrics_);
}
```
*   **Explanation**: This is the most important line in the pool. It asks the queue for a task. If the queue is empty, the thread safely waits (blocked) for 100ms before checking again.

---

## 3. `src/infrastructure/ThreadSafePriorityQueue.hpp`: The Data Safety

```cpp
// Line 29: Locking the Data
std::lock_guard<std::mutex> lock(mutex_);
queue_.push(std::move(item));
```
*   **Explanation**: We use a `lock_guard`. As soon as this function starts, it "locks the door." No other thread can touch the queue until this function finishes. This prevents the "Race Condition."

```cpp
// Line 33: Waking up Workers
cv_.notify_one();
```
*   **Explanation**: After adding a task, we shout to the sleeping workers: *"Hey! There's work to do!"* One lucky worker will wake up and grab the task.

---

## 4. `src/presentation/Dashboard.cpp`: The "Frontend" Logic

```cpp
// Line 98: Responsive Graphing
float graphW = (width_ - 80.0f) / 3.0f; 
float graphH = 220.0f;
```
*   **Explanation**: We don't use fixed numbers like `300px`. Instead, we take the current window `width_` and divide it by 3. This is why you can resize the window or go fullscreen and the UI doesn't break!

```cpp
// Line 188: Deduplication Logic
if (ut.id == it->id) { exists = true; break; }
```
*   **Explanation**: This ensures the "Priority Activity" strip only shows one box per task ID. We look backwards through the event log and only keep the latest state (Queued -> Running -> Done).

---

## 5. `src/engine/ScalingManager.cpp`: The AI Scaling

```cpp
// Line 35: The Decision Engine
int target = (queue_size / 2) + 2; 
pool_->scaleTo(target);
```
*   **Explanation**: This is the algorithm that decides the "Node Count." It's a linear scaling formula: for every 2 tasks in the queue, we want 1 worker, plus a baseline of 2.

---

## Important Terms for your Viva:
*   **RAII**: Resource Acquisition Is Initialization (How we use `lock_guard` to manage mutexes).
*   **Concurrency**: Multiple things happening at once.
*   **Throughput**: How many tasks are finished per second.
*   **Latency**: The delay between submitting a task and it finishing.
