// DNN 冒烟测试：验证 OpenCV 的 ONNX 加载与前向推理这条链路可以使用。

#include <opencv2/opencv.hpp>
#include <cstdio>

int main(int argc, char **argv) {
    if (argc < 2) {
        std::fprintf(stderr, "用法：%s <model.onnx>\n", argv[0]);
        return 2;
    }

    std::printf("OpenCV %s\n", CV_VERSION);

    cv::dnn::Net net;
    try {
        net = cv::dnn::readNetFromONNX(argv[1]);
    } catch (const std::exception &e) {
        std::fprintf(stderr, "readNetFromONNX 抛异常：%s\n", e.what());
        return 1;
    }
    if (net.empty()) {
        std::fprintf(stderr, "模型加载后 net 为空：%s\n", argv[1]);
        return 1;
    }

    // 用一张纯灰图跑通链路即可，不需要真实画面
    const cv::Mat dummy(640, 640, CV_8UC3, cv::Scalar(114, 114, 114));
    const cv::Mat blob = cv::dnn::blobFromImage(dummy, 1.0 / 255, cv::Size(),
                                                cv::Scalar(), true, false);

    cv::Mat out;
    try {
        net.setInput(blob);
        out = net.forward();
    } catch (const std::exception &e) {
        std::fprintf(stderr, "前向推理抛异常：%s\n", e.what());
        return 1;
    }

    std::printf("输出：dims=%d 形状=", out.dims);
    for (int i = 0; i < out.dims; ++i) {
        std::printf("%d%s", out.size[i], (i + 1 < out.dims) ? "x" : "\n");
    }

    if (out.dims != 3 || out.size[0] != 1 || out.size[1] != 84 || out.size[2] != 8400) {
        std::fprintf(stderr, "输出形状不是 YOLOv8n 期望的 [1, 84, 8400]"
                             " —— 后处理解码会因此出错\n");
        return 1;
    }

    std::printf("冒烟通过\n");
    return 0;
}
