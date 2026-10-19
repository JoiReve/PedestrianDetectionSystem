// FrameReaderFactory.cpp
#include "FrameReaderFactory.hpp"
#include "ImageFrameReader.hpp"
#include "VideoFrameReader.hpp"

#include <string>
#include <cctype>
#include <memory>
#include <stdexcept>

namespace {
    std::string convertExtensionToLower(const std::string &path) {
        const std::size_t dot = path.find_last_of('.');
        if (dot == std::string::npos || dot + 1 >= path.size()) {
            return {};
        }
        std::string extension = path.substr(dot + 1);
        for (char &c: extension) {
            if (c >= 'A' && c <= 'Z') {
                c = c - 'A' + 'a';
            }
        }
        return extension;
    }
} // namespace

SourceType detectSourceType(const std::string &path) {
    const std::string extension = convertExtensionToLower(path);
    if (extension.empty()) {
        return SourceType::Unknown;
    }

    // 图片后缀
    static const std::string kImageExtensions[] = {
        "jpg", "jpeg", "png", "bmp", "tif", "tiff", "webp", "ppm", "pgm"
    };
    for (const std::string &kie: kImageExtensions) {
        if (kie == extension) {
            return SourceType::Image;
        }
    }

    // 视频后缀
    static const char *kVideoExtensions[] = {
        "mp4", "avi", "mov", "mkv", "flv", "wmv", "webm", "m4v"
    };
    for (const std::string &kve: kVideoExtensions) {
        if (kve == extension) {
            return SourceType::Video;
        }
    }

    return SourceType::Unknown;
}


std::unique_ptr<FrameReader> createFrameReader(const ProjConfig &cfg) {
    switch (detectSourceType(cfg.source_path)) {
        case SourceType::Image:
            return std::make_unique<ImageFrameReader>(cfg);
        case SourceType::Video:
            return std::make_unique<VideoFrameReader>(cfg);
        case SourceType::Unknown:
        default:
            throw std::invalid_argument("无法识别的源类型：" + cfg.source_path);
    }
}
