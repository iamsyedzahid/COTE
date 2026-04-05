#include "infrastructure/Semaphore.hpp"

Semaphore::Semaphore(int initial_count) : count_(initial_count) {}

void Semaphore::acquire() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return count_ > 0; });
    --count_;
}

bool Semaphore::try_acquire() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (count_ <= 0) return false;
    --count_;
    return true;
}

void Semaphore::release() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++count_;
    }
    cv_.notify_one();
}
