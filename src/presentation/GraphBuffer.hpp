#pragma once
#include <deque>
#include <mutex>

class GraphBuffer {
public:
    explicit GraphBuffer(size_t capacity);

    void  push(float value);
    float max() const;
    float min() const;

    const std::deque<float>& data() const { return buffer_; }
    mutable std::mutex       mutex;

private:
    std::deque<float> buffer_;
    size_t            capacity_;
};
