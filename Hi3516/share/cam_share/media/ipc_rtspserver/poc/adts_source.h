/**
 * @FilePath     : adts_source.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-14 14:39:08
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 简单 ADTS 帧迭代器：从 ADTS 文件逐帧取出裸 AAC payload
 */

#pragma once

#include "ipc_rtsp/result.h"
#include "ipc_rtsp/types.h"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace ipc_rtsp
{
namespace detail
{

/** 按帧读取 ADTS 文件并剥离 7 字节头，产出裸 AAC 帧。 */
class AdtsFileSource
{
public:
    /** 整读文件到内存。
     *
     * @param path ADTS 文件路径。
     * @return 成功返回 Ok；打开失败 NotFound、空文件 InvalidArgument、读不完整 Internal。
     */
    Result Load(const char *path)
    {
        std::FILE *file = std::fopen(path, "rb");
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
        return Result::Ok();
    }

    /** 采样率（Hz）；解析失败返回 0。 */
    std::uint32_t sample_rate() const
    {
        return sample_rate_;
    }

    /** 声道数；解析失败返回 0。 */
    int channels() const
    {
        return channels_;
    }

    /** 取下一帧裸 AAC；循环播放：读完回到文件头继续；仅帧头解析/长度非法时返回 false。 */
    bool NextFrame(const std::uint8_t **payload, std::size_t *size)
    {
        if (offset_ + 7 > data_.size())
        {
            /* 循环播放。 */
            offset_ = 0;
            if (!ParseHeader(0))
            {
                return false;
            }
        }
        const std::size_t frame_len = FrameLength();
        if (frame_len < 8 || offset_ + frame_len > data_.size())
        {
            offset_ = 0;
            return false;
        }
        *payload = data_.data() + offset_ + 7u;
        *size = frame_len - 7u;
        offset_ += frame_len;
        return true;
    }

private:
    static const int kRates[13];

    /** 从当前 offset_ 读取 ADTS 帧长字段（13 位，不含 7 字节头）。
     *
     * @return 声明的整帧长度（字节）。
     */
    std::size_t FrameLength() const
    {
        const std::uint8_t *b = data_.data() + offset_;
        return (static_cast<std::size_t>(b[3] & 0x03u) << 11u) | (static_cast<std::size_t>(b[4]) << 3u) |
               (static_cast<std::size_t>(b[5] >> 5u));
    }

    /** 校验 at 处的 ADTS 同步字并解析采样率/声道数。
     *
     * @param at 帧头在 data_ 中的偏移。
     * @return 同步字合法返回 true；否则返回 false。
     */
    bool ParseHeader(std::size_t at)
    {
        if (at + 7 > data_.size())
        {
            return false;
        }
        const std::uint8_t *b = data_.data() + at;
        if (b[0] != 0xFF || (b[1] & 0xF0u) != 0xF0u)
        {
            return false;
        }
        const unsigned rate_index = (b[2] >> 2u) & 0x0Fu;
        if (rate_index < 13)
        {
            sample_rate_ = static_cast<std::uint32_t>(kRates[rate_index]);
        }
        channels_ = ((b[2] & 0x01u) << 2u) | ((b[3] >> 6u) & 0x03u);
        return true;
    }

    /* 整个文件的字节内容。 */
    std::vector<std::uint8_t> data_;
    /* 当前读取偏移（指向 ADTS 帧头）。 */
    std::size_t offset_ = 0;
    /* 头内解析出的采样率（Hz），解析失败为 0。 */
    std::uint32_t sample_rate_ = 0;
    /* 头内解析出的声道数，解析失败为 0。 */
    int channels_ = 0;
};

const int AdtsFileSource::kRates[13] = { 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350 };

} // namespace detail
} // namespace ipc_rtsp
