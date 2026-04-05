#include "infrastructure/Logger.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>

Logger::Logger() : log_file_("session_log.txt"), thread_(&Logger::processLoop, this) {}

Logger::~Logger() {
    running_ = false;
    cv_.notify_all();
    if (thread_.joinable()) thread_.join();
}

void Logger::log(LogLevel level, const std::string& message) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_.push(buildEntry(level, message));
    }
    cv_.notify_one();
}

void Logger::processLoop() {
    while (running_.load() || !pending_.empty()) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, std::chrono::milliseconds(50), [this] {
            return !pending_.empty() || !running_.load();
        });
        while (!pending_.empty()) {
            const std::string& entry = pending_.front();
            std::cout << entry << "\n";
            if (log_file_.is_open()) {
                log_file_ << entry << "\n";
                log_file_.flush();
            }
            pending_.pop();
        }
    }
}

std::string Logger::buildEntry(LogLevel level, const std::string& message) {
    auto now  = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
#ifdef _WIN32
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    char timebuf[16];
    std::strftime(timebuf, sizeof(timebuf), "%H:%M:%S", &tm_buf);

    const char* lvl = (level == LogLevel::INFO)  ? "INFO "
                    : (level == LogLevel::WARN)  ? "WARN "
                    :                              "ERROR";

    std::ostringstream oss;
    oss << "[" << timebuf << "] [" << lvl << "] " << message;
    return oss.str();
}
