// ImageFrameReader.hpp
#ifndef IMAGE_FRAME_READER_HPP
#define IMAGE_FRAME_READER_HPP

#include "FrameReader.hpp"
#include <string>

class ImageFrameReader final : public FrameReader {
public:
    ImageFrameReader(const ProjConfig &cfg);

    bool open() override;

    bool read(cv::Mat &frame) override;

    void release() override;

    bool isOpened() const override;

private:
    std::string image_path_;

    bool isOpened_ = false;

    bool isUsed_ = false;
};

#endif // IMAGE_FRAME_READER_HPP
