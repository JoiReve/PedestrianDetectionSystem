// FrameReader.hpp
#ifndef FRAME_READER_HPP
#define FRAME_READER_HPP

#include <opencv2/opencv.hpp>
#include "ProjConfig.hpp"

class FrameReader {
public:
    virtual bool open() = 0;

    virtual bool read(cv::Mat &frame) = 0;

    virtual void release() = 0;

    virtual bool isOpened() const = 0;

    virtual ~FrameReader() = default;
};

#endif //FRAME_READER_HPP
