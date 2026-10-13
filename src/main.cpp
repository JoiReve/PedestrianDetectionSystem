#include "PedestrianDetector.hpp"
#include "ProjConfig.hpp"
#include "FrameReaderFactory.hpp"

#include <iostream>
#include <cstdio>

#ifndef PROJECT_VERSION
#define PROJECT_VERSION "unknown"
#endif

namespace {
    constexpr auto kWindowName = "Pedestrian Detector";

    void drawDetections(cv::Mat &frame, const std::vector<Detection> &dts) {
        for (const auto &dt: dts) {
            cv::rectangle(frame, dt.box, cv::Scalar(0, 0, 255), 2);
            char buf[64];
            snprintf(buf, sizeof(buf), "person: %.2f", dt.confidence);
            cv::putText(frame, buf, cv::Point(dt.box.x, dt.box.y - 5), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                        cv::Scalar(0, 0, 255), 2);
        }
    }
} // namespace


int main(int argc, char **argv) {
        if (argc == 2 && std::string(argv[1]) == "--version") {
            std::cout << "PedestrianDetectionSystem " << PROJECT_VERSION << "\n";
        return 0;
    }

    if (argc < 3) {
        std::cerr << "用法：" << argv[0] << " <ONNX 路径> <源路径>" << "\n";
        return -1;
    }

    ProjConfig cfg;
    cfg.model_path = argv[1];
    cfg.source_path = argv[2];

    const SourceType type = detectSourceType(cfg.source_path);
    if (type == SourceType::Unknown) {
        std::cerr << "无法识别的源类型: " << cfg.source_path << "\n";
        return -1;
    }

    std::unique_ptr<FrameReader> fr;
    try {
        fr = createFrameReader(cfg);
    } catch (const std::exception &e) {
        std::cerr << "创建 fr 失败：" << e.what() << "\n";
        return -1;
    }

    if (!fr->open()) {
        std::cerr << "打开源失败：" << cfg.source_path << "\n";
        return -1;
    }

    std::unique_ptr<PedestrianDetector> pd;
    try {
        pd = std::make_unique<PedestrianDetector>(cfg);
    } catch (const std::exception &e) {
        std::cerr << "加载模型失败: " << e.what() << "\n";
        fr->release();
        return -1;
    }

    cv::Mat frame;
    const int delay = (type == SourceType::Image) ? 0 : 1;
    while (fr->read(frame)) {
        const auto d = pd->detect(frame);
        drawDetections(frame, d);
        cv::imshow(kWindowName, frame);
        const int key = cv::waitKey(delay);
        if (key == 27) {
            break;
        }
    }

    fr->release();
    cv::destroyAllWindows();
    return 0;
}
