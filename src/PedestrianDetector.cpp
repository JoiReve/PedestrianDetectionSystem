// PedestrianDetector.cpp
#include "PedestrianDetector.hpp"
#include <cmath>
#include <stdexcept>
#include "StageTimeTracker.hpp"

PedestrianDetector::PedestrianDetector(const ProjConfig &cfg)
    : cfg_(cfg) {
    // 加载 ONNX 模型
    net_ = cv::dnn::readNetFromONNX(cfg_.model_path);
    // 这里选择的是 OpenCV 后端 + CPU
    // 设置推理后端
    // net_.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    // 设置计算设备
    // net_.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
}

// 预处理
cv::Mat PedestrianDetector::preprocess(const cv::Mat &image, LetterBoxInfo &lb_info) const {
    if (image.empty()) {
        throw std::invalid_argument("输入图像不可为空！！！");
    }

    // 放缩预处理
    float scale_w = static_cast<float>(cfg_.input_width) / image.cols;
    float scale_h = static_cast<float>(cfg_.input_height) / image.rows;
    lb_info.scale = std::min(scale_h, scale_w);
    int new_w = static_cast<int>(std::round(image.cols * lb_info.scale));
    int new_h = static_cast<int>(std::round(image.rows * lb_info.scale));
    new_w = std::max(new_w, 1);
    new_h = std::max(new_h, 1);
    lb_info.pad_w = (cfg_.input_width - new_w) / 2;
    lb_info.pad_h = (cfg_.input_height - new_h) / 2;

    // 放缩
    cv::Mat resized;
    cv::resize(image, resized, cv::Size(new_w, new_h));

    // 填充
    cv::Mat letterbox_img(cfg_.input_height, cfg_.input_width, CV_8UC3, cv::Scalar(114, 114, 114));
    cv::Mat roi = letterbox_img(cv::Rect(lb_info.pad_w, lb_info.pad_h, new_w, new_h));
    resized.copyTo(roi);
    return letterbox_img;
}

// 后处理
std::vector<Detection> PedestrianDetector::postprocess(const cv::Mat &output, const LetterBoxInfo &lb_info,
                                                       const cv::Size &original_size) const {
    // 转置
    cv::Mat output_transposed;
    constexpr int kYoloV8OutputDim = 84;
    cv::transpose(output.reshape(1, kYoloV8OutputDim), output_transposed);

    const float *data = reinterpret_cast<float *>(output_transposed.data);
    const int num_candidates = output_transposed.rows;
    const int num_classes = output_transposed.cols - 4;

    // 计算框的位置、类型、置信度
    std::vector<cv::Rect> boxes;
    std::vector<float> confidences;
    for (int i = 0; i < num_candidates; ++i) {
        cv::Mat scores(1, num_classes, CV_32FC1, const_cast<float *>(data + i * output_transposed.cols + 4));
        cv::Point class_id;
        double max_score = 0.0;
        cv::minMaxLoc(scores, nullptr, &max_score, nullptr, &class_id);
        if (max_score < cfg_.conf_threshold || class_id.x != cfg_.person_class_id) {
            continue;
        }
        float cx = *(data + (i * output_transposed.cols + 0));
        float cy = *(data + (i * output_transposed.cols + 1));
        float w = *(data + (i * output_transposed.cols + 2));
        float h = *(data + (i * output_transposed.cols + 3));

        // 等比例还原检测框
        float orig_cx = (cx - lb_info.pad_w) / lb_info.scale;
        float orig_cy = (cy - lb_info.pad_h) / lb_info.scale;
        float orig_w = w / lb_info.scale;
        float orig_h = h / lb_info.scale;
        int left = std::max(0, static_cast<int>(orig_cx - orig_w / 2.0f));
        int top = std::max(0, static_cast<int>(orig_cy - orig_h / 2.0f));
        int right = std::min(original_size.width, static_cast<int>(orig_cx + orig_w / 2.0f));
        int bottom = std::min(original_size.height, static_cast<int>(orig_cy + orig_h / 2.0f));
        boxes.emplace_back(cv::Rect(left, top, right - left, bottom - top));
        confidences.emplace_back(static_cast<float>(max_score));
    }

    std::vector<Detection> detections;
    // NMS 去重
    std::vector<int> nms_indices;
    cv::dnn::NMSBoxes(boxes, confidences, cfg_.conf_threshold, cfg_.iou_threshold, nms_indices);
    for (int idx: nms_indices) {
        Detection det;
        det.box = boxes[idx];
        det.confidence = confidences[idx];
        det.class_id = cfg_.person_class_id;
        detections.emplace_back(det);
    }
    return detections;
}

// 接口
std::vector<Detection> PedestrianDetector::detect(const cv::Mat &image) {
    LetterBoxInfo lb_info;
    cv::Mat blob;
    cv::Mat output;
    std::vector<Detection> result;

    {
        STAGE_TIME_TRACKER_SCOPE("预处理");
        // 预处理
        cv::Mat input_blob = preprocess(image, lb_info);

        // net_
        cv::dnn::blobFromImage(
            input_blob,
            blob,
            1.0 / 255,
            cv::Size(),
            cv::Scalar(0, 0, 0),
            true,
            false
        );
    }

    {
        STAGE_TIME_TRACKER_SCOPE("推理");
        // 前向推理
        net_.setInput(blob);
        output = net_.forward();
    }

    {
        STAGE_TIME_TRACKER_SCOPE("后处理");
        // 后处理
        result = postprocess(output, lb_info, image.size());
        return result;
    }
}
