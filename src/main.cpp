// main.cpp
#include "PedestrianDetector.hpp"
#include "ProjConfig.hpp"
#include "FrameReaderFactory.hpp"
#include "StageTimeTracker.hpp"

#include <iostream>
#include <cstdio>
#include <fstream>
#include <iomanip>

#ifndef PROJECT_VERSION
#define PROJECT_VERSION "unknown"
#endif

// 画检测框
namespace {

constexpr auto kWindowName = "Pedestrian Detector";

void drawDetections(cv::Mat &frame, const std::vector<Detection> &dts) {
    STAGE_TIME_TRACKER_SCOPE("Draw");
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

    bool no_show = false;
    std::string dump_csv = "";
    for(int i = 3;i < argc; ++i) {
        std::string arg = argv[i];
        if(arg == "--no-show") {
            no_show = true;
        } else if(arg == "--dump" && i + 1 < argc) {
            dump_csv = argv[++i];
        }
    }

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

    std::ofstream csv_file;
    if(!dump_csv.empty()) {
        csv_file.open(dump_csv);
        if(csv_file.is_open()) {
            csv_file << "frame_id,x,y,w,h,score\n";
        }
    }

    cv::Mat frame;
    const int delay = (type == SourceType::Image) ? 0 : 1;
    int frame_idx = 0;
    while (true) {
        {
            STAGE_TIME_TRACKER_SCOPE("EndToEnd");
            bool read_success = false;
            {
                STAGE_TIME_TRACKER_SCOPE("Read");
                read_success = fr->read(frame);
            }
            if(!read_success || frame.empty()) {
                break;
            }
            std::vector<Detection> d;
            {
                STAGE_TIME_TRACKER_SCOPE("InferChain");
                d = pd->detect(frame);
            }

            if(csv_file.is_open()) {
                if(d.empty()) {
                    csv_file << frame_idx << ",0,0,0,0,0.000\n";
                } else {
                    for(const auto& tmp : d) {
                        csv_file << frame_idx << ","
                                 << (int)std::round(tmp.box.x) << ","
                                 << (int)std::round(tmp.box.y) << ","
                                 << (int)std::round(tmp.box.width) << ","
                                 << (int)std::round(tmp.box.height) << ","
                                 << std::fixed << std::setprecision(3) << tmp.confidence << "\n";
                    }
                }
            }

            drawDetections(frame, d);
            cv::Mat display_frame;
            double max_width = 1280.0;
            if (frame.cols > max_width) {
                double scale = max_width / frame.cols;
                cv::resize(frame, display_frame, cv::Size(), scale, scale);
            } else {
                display_frame = frame;
            }
            int key = -1;
            if(!no_show) {
                {
                    STAGE_TIME_TRACKER_SCOPE("DisplayNWait");
                    cv::imshow(kWindowName, display_frame);
                    key = cv::waitKey(delay);
                }
            } else {
                cv::waitKey(1);
            }
            if (key == 27 || key == 'q' || key == 'Q') {
                break;
            }
        }
        if (frame_idx == 0) {
            std::cout << "\n>>> [Cold Start] Frame 1 (包含初始化与窗口创建开销) <<<\n";
        } else {
            std::cout << "\n>>> [Steady State] Frame " << frame_idx + 1 << " <<<\n";
        }
        StageTimeTracker::getInstance().printReport();
        frame_idx++;
    }
    if (csv_file.is_open()) {
        csv_file.close();
        std::cout << "\n[Golden Snapshot Saved] -> " << dump_csv << "\n";
    }
    fr->release();
    cv::destroyAllWindows();
    return 0;
}
