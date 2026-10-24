// PedestrianDetector.hpp
#ifndef PEDESTRIAN_DETECTOR_HPP
#define PEDESTRIAN_DETECTOR_HPP

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <string>
#include <vector>
#include "ProjConfig.hpp"

// 缩放的相关信息
struct LetterBoxInfo {
    float scale;
    int pad_w;
    int pad_h;
};

// 输出内容
struct Detection {
    cv::Rect box;
    float confidence;
    int class_id;
};

class PedestrianDetector {
private:
    cv::dnn::Net net_;

    const ProjConfig cfg_;

public:
    PedestrianDetector(const ProjConfig &cfg);

    std::vector<Detection> detect(const cv::Mat &image);

private:
    // 预处理
    cv::Mat preprocess(const cv::Mat &image, LetterBoxInfo &lb_info) const;

    // 经过预处理之后，会 return blob
    // blob 是图像经过预处理后，转成网络能使用的多维数组

    // 后处理
    std::vector<Detection> postprocess(const cv::Mat &output, const LetterBoxInfo &lb_info,
                                       const cv::Size &original_size) const;
};

#endif // PEDESTRIAN_DETECTOR_HPP
