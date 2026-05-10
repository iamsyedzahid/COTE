# Complete File-by-File Explanation (Viva Cheat Sheet) 📘

This document provides a function-by-function breakdown of every file in the `src` directory.

---

## 📂 1. Infrastructure (The System Foundation)

### `Logger.cpp / .hpp`

- **`Logger()`**: Constructor. Opens `session_log.txt` and starts a background thread.
- **`log()`**: Adds a message to a queue and notifies the background thread. It uses a **Mutex** to ensure thread-safety.
- **`processLoop()`**: The background thread. It waits for messages, prints them to the console, and writes them to the file.
- **`buildEntry()`**: Formats the log message with a timestamp (HH:MM:SS) and level (INFO/WARN).

### `MetricsService.cpp / .hpp`

- **`snapshot()`**: Captures the current state of the simulation (running tasks, queue size, latency) and returns a "Snapshot" object.
- **`recordCompletion()`**: Calculates the average latency by keeping a rolling history of the last 100 finished tasks.
- **`recordEvent()`**: Stores a task state change (e.g., QUEUED to RUNNING). We increased its capacity to 50 events for better tracking.

### `Semaphore.cpp / .hpp`

- **`acquire()`**: Decrements the counter. If the counter is 0, the thread blocks (sleeps) until someone calls `release()`.
- **`release()`**: Increments the counter and wakes up a waiting thread.

### `ThreadSafePriorityQueue.hpp`

- **`push()`**: Adds an item and calls `notify_one()` to wake up a worker.
- **`wait_and_pop()`**: The most important function. It makes a worker thread wait until a task is available, then safely removes the highest-priority task.

---

## 📂 2. Engine (The Orchestration Logic)

### `WorkerPool.cpp / .hpp`

- **`spawnWorker()`**: Creates a new `std::thread` and adds it to the pool.
- **`workerLoop()`**: The "infinite loop" inside every worker thread. It handles the **Pause** logic and calls the `TaskExecutor`.
- **`scaleTo()`**: Dynamically adds or removes threads to reach a target number.

### `ScalingManager.cpp / .hpp`

- **`monitorLoop()`**: Runs every 500ms. It calculates how many workers are needed based on the current `queue_size`.
- **Logic**: If queue size is huge, it triggers `scaleUp()`.

### `TaskExecutor.cpp / .hpp`

- **`execute()`**: Runs the actual workload of a task. It starts a timer, executes the function, and then reports the duration/latency to the `MetricsService`.

### `TaskScheduler.cpp / .hpp`

- **`submit()`**: Receives a raw task from the service, logs it as "QUEUED," and pushes it into the priority queue.

---

## 📂 3. Services (The Management Layer)

### `TaskService.cpp / .hpp`

- **`submit()`**: The primary API. It generates a unique Task ID, creates a `Task` object, and hands it over to the scheduler.

### `MonitoringService.cpp / .hpp`

- **`reportLoop()`**: Every 1 second, it takes a metrics snapshot and sends a formatted string to the `Logger` (e.g., "METRICS | Queued=5 Running=2...").

---

## 📂 4. Presentation (The UI)

### `Dashboard.cpp / .hpp`

- **`Dashboard()`**: Constructor. Initializes the Raylib window and loads the font (with a crash-protection check).
- **`render()`**: The heart of the UI. It calls specific sub-functions to draw each part of the screen.
- **`drawGraph()`**: Takes the history data and draws lines using `DrawLineEx`. It handles "Scaling" so the graph always fits the card.
- **`drawActivityStrip()`**: Displays the tasks as colorful boxes. It includes the **Deduplication Logic** to only show the latest state per task.

---

## 📂 5. Models (The Data Structures)

### `Task.hpp`

- Contains the `Task` struct. It stores the ID, Priority, Workload (the function to run), and timestamps.

### `MetricsSnapshot.hpp`

- A "Plain Old Data" (POD) struct used to move all statistics from the background threads to the UI thread in one safe package.

---

## 📂 6. Root

### `main.cpp`

- **`main()`**: The "God Function." It instantiates every single class, starts the threads, simulates the clients, and runs the final UI loop.
- **Keyboard Handling**: Contains the logic for F11 (Fullscreen), SPACE (Pause), B (Burst), and T (Manual Task).
