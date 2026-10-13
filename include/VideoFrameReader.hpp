// VideoFrameReader.hpp

#ifndef VIDEO_FRAME_READER_HPP
#define VIDEO_FRAME_READER_HPP

#include "FrameReader.hpp"
#include <string>

class VideoFrameReader : public FrameReader {
public:
    VideoFrameReader(const ProjConfig &cfg);

    bool open() override;

    bool read(cv::Mat &frame) override;

    void release() override;

    bool isOpened() const override;

private:
    cv::VideoCapture cap_;

    std::string video_path_;

    bool isOpened_ = false;
};

#endif // VIDEO_FRAME_READER_HPP
