/**
 * @FilePath     : jpeg_file_source.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-14 15:21:46
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : Motion-JPEG 文件媒体源实现
 */

#include "media/jpeg_file_source.h"

#include <cstdio>

namespace ipc_rtsp
{
namespace detail
{

Result JpegFileSource::Load(const std::string &path)
{
    std::FILE *file = std::fopen(path.c_str(), "rb");
    if (file == nullptr)
    {
        return Result::Fail(Status::NotFound);
    }
    std::fseek(file, 0, SEEK_END);
    const long size = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);
    if (size <= 0)
    {
        std::fclose(file);
        return Result::Fail(Status::InvalidArgument);
    }
    data_.resize(static_cast<std::size_t>(size));
    const std::size_t read = std::fread(data_.data(), 1, data_.size(), file);
    std::fclose(file);
    if (read != data_.size())
    {
        return Result::Fail(Status::Internal);
    }

    frames_.clear();
    std::size_t i = 0;
    while (i + 2 <= data_.size())
    {
        if (data_[i] != 0xFF || data_[i + 1] != 0xD8)
        {
            ++i;
            continue;
        }
        /* 找 SOI 后向后找配对 EOI；熵数据内的 0xFF 已被转义，非转义 FFD9 即帧尾。 */
        std::size_t j = i + 2;
        while (j + 2 <= data_.size())
        {
            if (data_[j] == 0xFF && data_[j + 1] == 0xD9)
            {
                break;
            }
            ++j;
        }
        if (j + 2 > data_.size())
        {
            break;
        }
        FrameRange range;
        range.offset = i;
        range.size = j + 2 - i;
        frames_.push_back(range);
        i = j + 2;
    }

    if (frames_.empty())
    {
        return Result::Fail(Status::InvalidArgument);
    }
    cursor_ = 0;
    return Result::Ok();
}

bool JpegFileSource::NextAu(VideoFrameView *out)
{
    if (frames_.empty() || cursor_ >= frames_.size())
    {
        if (!loop_ || frames_.empty())
        {
            return false;
        }
        cursor_ = 0;
    }

    const FrameRange &range = frames_[cursor_++];
    out->data = data_.data() + range.offset;
    out->size = range.size;
    out->codec = Codec::MJPEG;
    out->first_nal = NalUnitType::Unknown;
    out->has_pts = false; /* 由 PoC 按帧率补齐时间戳 */
    out->pts_us = 0;
    /* 每帧 JPEG 都是独立可解码的关键帧。 */
    out->key = true;
    return true;
}

} // namespace detail
} // namespace ipc_rtsp
