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

namespace {

constexpr auto kWindowName = "Pedestrian Detector";

constexpr double kMaxDisplayWidth = 1280.0;

struct ProjOptions{
    std::string model_path;
    std::string source_path;
    std::string dump_csv_path;  // 导出 CSV 的路径
    bool no_show = false;       // 无头模式
};

enum class ParseResult {
    Success,    // 不需要进入业务
    Continue,   // 需要进入业务，后面要进入 run 函数
    Failure,
};

// 命令行用法展示
void printUsage(std::ostream& out, const char* program) {
    out << "Usage: " << program << " [Options] <Model Path> <Source Path>\n"
        << "\n"
        << "Options:\n"
        << "  -h, --help              display this help and exit\n"
        << "  -v, --version           output version information and exit\n"
        << "  --no-show               do not open a display window\n"
        << "  --dump <CSV Path>       write per-frame detections to CSV\n";
}

// 解析命令行
ParseResult parseArgs(int argc, char** argv, ProjOptions& po) {
    std::vector<std::string> Position;
    for(int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        // -h, --help
        if (arg == "-h" || arg == "--help") {
            printUsage(std::cout, argv[0]);
            return ParseResult::Success;
        }
        // -v, --version
        if (arg == "-v" || arg == "--version") {
            std::cout << "PedestrianDetectionSystem " << PROJECT_VERSION << "\n";
            return ParseResult::Success;
        }
        // --no-show, --dump
        if (arg == "--no-show") {
            po.no_show = true;
        } else if (arg == "--dump") {
            if (i + 1 >= argc) {
                std::cerr << argv[0] << ": option '--dump' requires an argument\n";
                std::cerr << "Try '" << argv[0] << " -h / --help' for more information.\n";
                return ParseResult::Failure;
            }
            po.dump_csv_path = argv[++i];
        } else if (!arg.empty() && arg.front() == '-') {
            std::cerr << "Error: unknown option " << arg << '\n';
            std::cerr << "Try '" << argv[0] << " -h / --help' for more information.\n";
            return ParseResult::Failure;
        } else {
            Position.push_back(arg);
        }
    }

    // 少于两个放在了出错的情况在 run 函数中
    if(Position.size() != 2) {
        std::cerr << "Error: expected 2 positional arguments "
                  << "<Model Name> <Source Name>, got " << Position.size() << '\n';
        std::cerr << "Try '" << argv[0] << " --help' for more information.\n";
        return ParseResult::Failure;
    }
    po.model_path  = Position[0];
    po.source_path = Position[1];
    return ParseResult::Continue;
}

// 画检测框
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

// 修正检测框的大小
cv::Mat adjustSize(const cv::Mat& frame){
    cv::Mat display_frame;
    if (frame.cols > kMaxDisplayWidth) {
        double scale = kMaxDisplayWidth / frame.cols;
        cv::resize(frame, display_frame, cv::Size(), scale, scale);
    } else {
        display_frame = frame;
    }
    return display_frame;
}

// CSV 快照
class CSVSnapshot {
private:
    // 将数据输出到文件中
    std::ofstream file_;

public:
    // 构造即完成打开加写表头
    explicit CSVSnapshot(const std::string& path) {
        // 空路径只是不写 CSV 文件，而不是错误
        if(path.empty()) {
            return;
        }
        file_.open(path);
        if(!isOpen()) {
            std::cerr << "Error: cannot open CSV file " << path << '\n';
            return;
        }
        file_ << "id,x,y,w,h,score\n";
    }

    ~CSVSnapshot() {
        if(file_.is_open()) {
            file_.close();
        }
    }

    bool isOpen() const {
        return file_.is_open();
    }

    void writeCSV(const std::vector<Detection>& d, int id) {
        if(!file_.is_open()) {
            return;
        }
        if(d.empty()) {
            file_ << id << ",0,0,0,0,0.000\n";
            return;
        }
        for(const auto& tmp : d) {
            file_ << id << ","
                  << (int)std::round(tmp.box.x) << ","
                  << (int)std::round(tmp.box.y) << ","
                  << (int)std::round(tmp.box.width) << ","
                  << (int)std::round(tmp.box.height) << ","
                  << std::fixed << std::setprecision(3) << tmp.confidence << "\n";
        }
    }
};

// 输出性能检测数据
void printFrameReport(int id) {
    if(id == 0) {
        std::cout << "\n>>> [Cold Start] Frame 1 (init + window creation included) <<<\n";
    } else {
        std::cout << "\n>>> [Steady State] Frame " << id + 1 << " <<<\n";
    }
    StageTimeTracker::getInstance().printReport();
}

int run(const ProjOptions& po) {
    ProjConfig cfg;
    cfg.model_path = po.model_path;
    cfg.source_path = po.source_path;
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
    CSVSnapshot cs(po.dump_csv_path);
    cv::Mat frame;
    const int delay = (type == SourceType::Image) ? 0 : 1;
    int id = 0;
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

            cs.writeCSV(d, id);
            
            int key = -1;
            if(!po.no_show) {
                drawDetections(frame, d);
                {
                    STAGE_TIME_TRACKER_SCOPE("DisplayNWait");
                    cv::imshow(kWindowName, adjustSize(frame));
                    key = cv::waitKey(delay);
                }
            } else {
                cv::waitKey(1);
            }
            if (key == 27 || key == 'q' || key == 'Q') {
                break;
            }
        }
        printFrameReport(id);
        ++id;
    }
    fr->release();
    cv::destroyAllWindows();
    return 0;
}

} // namespace


int main(int argc, char **argv) {
    ProjOptions po;
    switch(parseArgs(argc, argv, po)) {
        case ParseResult::Success:
            return 0;
        case ParseResult::Failure:
            return 1;
        case ParseResult::Continue:
            break;   
    }
    return run(po);
}