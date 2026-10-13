// FrameReaderFactory.hpp
#ifndef FRAME_READER_FACTORY_HPP
#define FRAME_READER_FACTORY_HPP

#include "FrameReader.hpp"

enum class SourceType {
    Image,
    Video,
    Unknown
};

SourceType detectSourceType(const std::string &path);

std::unique_ptr<FrameReader> createFrameReader(const ProjConfig &cfg);

#endif // FRAME_READER_FACTORY_HPP
