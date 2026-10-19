// ImageFrameReader.cpp
#include "ImageFrameReader.hpp"
#include <iostream>

ImageFrameReader::ImageFrameReader(const ProjConfig &cfg)
    : image_path_(cfg.source_path) {
}

bool ImageFrameReader::open() {
    if (image_path_.empty()) {
        std::cerr << "路径：" << image_path_ << "为空！！！" << std::endl;
        return false;
    }
    isOpened_ = true;
    isUsed_ = false;
    return true;
}

bool ImageFrameReader::read(cv::Mat &frame) {
    if (!isOpened_ || isUsed_) {
        return false;
    }
    frame = cv::imread(image_path_);
    isUsed_ = true;
    return true;
}

void ImageFrameReader::release() {
    isOpened_ = false;
    isUsed_ = false;
}

bool ImageFrameReader::isOpened() const {
    return isOpened_;
}