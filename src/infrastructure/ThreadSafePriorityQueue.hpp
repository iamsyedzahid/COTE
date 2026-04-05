#pragma once
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>

template<typename T, typename Compare = std::less<T>>
class ThreadSafePriorityQueue {
public:
    void push(T item);
    bool try_pop(T& out);
    bool wait_and_pop(T& out, std::chrono::milliseconds timeout);
    size_t size() const;
    bool   empty() const;
    void   stop();

private:
    mutable std::mutex                          mutex_;
    std::condition_variable                     cv_;
    std::priority_queue<T, std::vector<T>, Compare> queue_;
    std::atomic<bool>                           stopped_{false};
};

template<typename T, typename Compare>
void ThreadSafePriorityQueue<T, Compare>::push(T item) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopped_) return;
        queue_.push(std::move(item));
    }
    cv_.notify_one();
}

template<typename T, typename Compare>
bool ThreadSafePriorityQueue<T, Compare>::try_pop(T& out) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) return false;
    out = queue_.top();
    queue_.pop();
    return true;
}

template<typename T, typename Compare>
bool ThreadSafePriorityQueue<T, Compare>::wait_and_pop(T& out, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait_for(lock, timeout, [this] {
        return !queue_.empty() || stopped_.load();
    });
    if (queue_.empty()) return false;
    out = queue_.top();
    queue_.pop();
    return true;
}

template<typename T, typename Compare>
size_t ThreadSafePriorityQueue<T, Compare>::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

template<typename T, typename Compare>
bool ThreadSafePriorityQueue<T, Compare>::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

template<typename T, typename Compare>
void ThreadSafePriorityQueue<T, Compare>::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopped_ = true;
    }
    cv_.notify_all();
}
