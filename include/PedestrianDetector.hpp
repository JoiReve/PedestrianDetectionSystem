// PedestrianDetector.hpp
#ifndef PEDESTRIAN_DETECTOR_HPP
#define PEDESTRIAN_DETECTOR_HPP

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <string>
#include <vector>

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
public:
    PedestrianDetector(const std::string &model_path, float conf_threshold = 0.5f, float iou_threshold = 0.45f);

    std::vector<Detection> detect(const cv::Mat &image);

private:
    cv::dnn::Net net_;
    float conf_threshold_;
    float iou_threshold_;
    int input_width_ = 640;
    int input_height_ = 640;
    int person_class_id_ = 0;

    // 预处理
    cv::Mat preprocess(const cv::Mat &image, LetterBoxInfo &lb_info);

    // 经过预处理之后，会 return blob
    // blob 是图像经过预处理后，转成网络能使用的多维数组

    // 后处理
    std::vector<Detection> postprocess(const cv::Mat &output, const LetterBoxInfo &lb_info,
                                       const cv::Size &original_size);
};

#endif // PEDESTRIAN_DETECTOR_HPP
