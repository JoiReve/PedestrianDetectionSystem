#include "PedestrianDetector.hpp"
#include <iostream>
#include <cstdio>

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "用法：" << argv[0] << "<.onnx 文件> <图片路径>" << std::endl;
        return -1;
    }
    const std::string model_path = argv[1];
    const std::string image_path = argv[2];

    cv::Mat image = cv::imread(image_path);
    if (image.empty()) {
        std::cerr << "路径：" << image_path << "无法加载！！！" << std::endl;
        return -1;
    }

    try {
        PedestrianDetector pd(model_path);
        auto detections = pd.detect(image);

        for (const auto &d: detections) {
            cv::rectangle(image, d.box, cv::Scalar(0, 0, 255), 2);
            // std::string label = "person" + std::to_string(d.confidence).substr(0, 4);
            char buf[64];
            std::snprintf(buf, sizeof(buf), "person %.2f", d.confidence);
            std::string label = buf;
            cv::putText(image, label, cv::Point(d.box.x, d.box.y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.6,
                        cv::Scalar(0, 0, 255), 2);
        }
        cv::imshow("output_image", image);
        cv::waitKey(0);
        cv::destroyAllWindows();
    } catch (const std::exception &e) {
        std::cerr << "错误：" << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
