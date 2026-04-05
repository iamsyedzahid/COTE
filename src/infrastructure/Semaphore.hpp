#pragma once
#include <mutex>
#include <condition_variable>

class Semaphore {
public:
    explicit Semaphore(int initial_count);

    void acquire();
    bool try_acquire();
    void release();

private:
    std::mutex              mutex_;
    std::condition_variable cv_;
    int                     count_;
};
