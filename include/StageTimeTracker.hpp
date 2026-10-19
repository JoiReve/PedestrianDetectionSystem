// StageTimeTracker.hpp
#ifndef STAGE_TIME_TRACKER_HPP
#define STAGE_TIME_TRACKER_HPP

#include <map>
#include <string>
#include <mutex>
#include <string>
#include <chrono>

// StageTimeTracke Class
class StageTimeTracker{
private:
    StageTimeTracker() = default;

    std::map<std::string, double> stats_;   // 累计时间

    std::map<std::string, double> cnts_;    // 累计次数

    std::mutex mutex_;                      // 互斥锁

public:
    static StageTimeTracker& getInstance();

    void addTime(const std::string& name, double ms);

    void printReport();
};

// ScopedTimer Class
class ScopedTimer {
public:
    explicit ScopedTimer(const std::string& name);

    ~ScopedTimer();

    // 拷贝构造、移动构造全部禁止
    ScopedTimer(const ScopedTimer&) = delete;

    ScopedTimer& operator=(const ScopedTimer&) = delete;

    ScopedTimer(ScopedTimer&&) = delete;

    ScopedTimer& operator=(ScopedTimer&&) = delete;

private:
    std::string name_;

    std::chrono::time_point<std::chrono::steady_clock> start_;
};

// 创建一个用于计时的对象
#define STAGE_TIME_TRACKER_CONCAT_INNER(a, b) a##b
#define STAGE_TIME_TRACKER_CONCAT(a, b) STAGE_TIME_TRACKER_CONCAT_INNER(a, b)
#define STAGE_TIME_TRACKER_SCOPE(name) ScopedTimer STAGE_TIME_TRACKER_CONCAT(_timer_, __LINE__)(name)

#endif // STAGE_TIME_TRACKER_HPP