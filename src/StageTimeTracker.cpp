#include "StageTimeTracker.hpp"
#include <cstdio>
#include <iostream>

// StageTimeTracker Class
StageTimeTracker& StageTimeTracker::getInstance() {
    static StageTimeTracker instance;
    return instance;
}

void StageTimeTracker::addTime(const std::string& name, double ms) {
    std::lock_guard<std::mutex> lg(mutex_);
    stats_[name] += ms;
    cnts_[name]++;
}

void StageTimeTracker::printReport() {
    std::lock_guard<std::mutex> lg(mutex_);
    if(cnts_.empty()) {
        return;
    }
    double totalTime = 0.0;
    for(const auto& s : stats_) {
        totalTime += s.second;
    }

    std::cout << "\nv0.2.2 Performance Metrics Report\n";
    printf("%-15s%-12s%-12s%s\n", "Stage", "Total(ms)", "Avg(ms)", "Ratio");

    for(const auto& s : stats_) {
        const std::string& name = s.first;
        double total_ms = s.second;
        double avg_ms = total_ms / cnts_.at(name);
        double percent = (totalTime > 0) ? (total_ms / totalTime * 100.0) : 0.0;
        printf("%-15s%-12.2f%-12.2f%.2f%%\n", name.c_str(), total_ms, avg_ms, percent);
    }
    std::cout << "Total Time Spent: " << totalTime << '\n';
    stats_.clear();
    cnts_.clear();
}

// ScopedTimer Class
ScopedTimer::ScopedTimer(const std::string& name)
    : name_(name), start_(std::chrono::steady_clock::now()) {  }

ScopedTimer::~ScopedTimer() {
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> diff = end - start_;
    StageTimeTracker::getInstance().addTime(name_, diff.count());
}
