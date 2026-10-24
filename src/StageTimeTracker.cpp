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
    cnts_[name]++;
    for(auto& pair : stats_) {
        if(pair.first == name) {
            pair.second += ms;
            return;
        }
    }
    stats_.emplace_back(name, ms);

}

void StageTimeTracker::printReport() {
    std::lock_guard<std::mutex> lg(mutex_);
    if(cnts_.empty()) {
        return;
    }
    double totalTime = 0.0;
    for(const auto& s : stats_) {
        if (s.first == "EndToEnd"){
            totalTime += s.second;  
            break;
        }
    }

    std::cout << "\nPerformance Metrics Report\n";
    printf("%-20s%-12s%-12s%s\n", "Stage", "Total(ms)", "Avg(ms)", "Ratio(%)");

    std::vector<std::string> order = {
        "EndToEnd", 
        "Read", 
        "InferChain", 
        "Preprocess", 
        "Reason", 
        "Postprocess",
        "Draw", 
        "DisplayNWait"
    };
    
    for (const auto& o : order) {
        for(const auto& s : stats_) {
            if (s.first == o) {
                double total_ms = s.second;
                double avg_ms = total_ms / cnts_.at(o);
                double percent = (totalTime > 0) ? (total_ms / totalTime * 100.0) : 0.0;
                std::string display_name = o;
                if(o == "Read" || o == "InferChain" || o == "Draw" || o == "DisplayNWait") {
                    display_name = "  " + o;
                } else if(o == "Preprocess" || o == "Reason" || o == "Postprocess") {
                    display_name = "    " + o;
                }
                printf("%-20s%-12.2f%-12.2f%.2f%%\n", display_name.c_str(), total_ms, avg_ms, percent);
                break;
            }
        }
    }
    
    std::cout << "Total Frame Latency(ms): " << totalTime << '\n';
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
