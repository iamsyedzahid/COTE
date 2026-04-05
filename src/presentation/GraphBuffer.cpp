#include "presentation/GraphBuffer.hpp"
#include <algorithm>
#include <limits>

GraphBuffer::GraphBuffer(size_t capacity) : capacity_(capacity) {}

void GraphBuffer::push(float value) {
    if (buffer_.size() >= capacity_) buffer_.pop_front();
    buffer_.push_back(value);
}

float GraphBuffer::max() const {
    if (buffer_.empty()) return 1.0f;
    return *std::max_element(buffer_.begin(), buffer_.end());
}

float GraphBuffer::min() const {
    if (buffer_.empty()) return 0.0f;
    return *std::min_element(buffer_.begin(), buffer_.end());
}
