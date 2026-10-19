// VideoFrameReader.cpp
#include "VideoFrameReader.hpp"
#include <iostream>

VideoFrameReader::VideoFrameReader(const ProjConfig &cfg)
    : video_path_(cfg.source_path) {
}

bool VideoFrameReader::open() {
    if (video_path_.empty()) {
        std::cerr << "路径：" << video_path_ << "为空！！！" << std::endl;
        return false;
    }

    if (!cap_.open(video_path_)) {
        std::cerr << "路径：" << video_path_ << "无法打开！！！" << std::endl;
        return false;
    }

    isOpened_ = true;
    return true;
}

bool VideoFrameReader::read(cv::Mat &frame) {
    if (!isOpened_) {
        return false;
    }
    bool isRead = cap_.read(frame);
    return isRead;
}

void VideoFrameReader::release() {
    if (cap_.isOpened()) {
        cap_.release();
    }
    isOpened_ = false;
}

bool VideoFrameReader::isOpened() const {
    return isOpened_;
}
