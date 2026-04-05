#pragma once
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <fstream>

enum class LogLevel { INFO, WARN, ERROR };

class Logger {
public:
    Logger();
    ~Logger();

    void log(LogLevel level, const std::string& message);

private:
    void        processLoop();
    static std::string buildEntry(LogLevel level, const std::string& message);

    std::queue<std::string> pending_;
    std::mutex              mutex_;
    std::condition_variable cv_;
    std::atomic<bool>       running_{true};
    std::ofstream           log_file_;
    std::thread             thread_;
};
