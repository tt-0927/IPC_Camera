/**
 * @FilePath     : raw_audio_source.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-14 15:21:46
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 定长裸音频帧源：G711/G726 等无帧头格式按固定字节切分
 */

/*
 * 板端 AENC 的 G711/G726 输出是定长帧（如 8kHz/25fps 时 G711 每帧 320 字节、
 * G726-32 每帧 160 字节），裸文件与之同构，直接按帧长切分即可。
 */
#pragma once

#include "ipc_rtsp/result.h"

#include <cstdint>
#include <cstdio>
#include <vector>

namespace ipc_rtsp
{
namespace detail
{

/** 按固定字节帧长读取裸音频文件（G711a/G711u/G726）。 */
class RawAudioFileSource
{
public:
    /** 整读文件到内存。
     *
     * @param path 裸音频文件路径。
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

    /** 单帧字节数（须与源文件编码参数匹配：8kHz/25fps 时 G711 为 320）。 */
    void set_frame_bytes(std::size_t bytes)
    {
        frame_bytes_ = bytes > 0 ? bytes : 320;
    }

    /** 取下一帧；文件尾部不足一帧的余量丢弃后循环。 */
    bool NextFrame(const std::uint8_t **payload, std::size_t *size)
    {
        if (frame_bytes_ == 0 || data_.empty())
        {
            return false;
        }
        if (offset_ + frame_bytes_ > data_.size())
        {
            offset_ = 0;
        }
        *payload = data_.data() + offset_;
        *size = frame_bytes_;
        offset_ += frame_bytes_;
        return true;
    }

private:
    /* 整个文件的字节内容。 */
    std::vector<std::uint8_t> data_;
    /* 当前读取偏移。 */
    std::size_t offset_ = 0;
    /* 单帧字节数（默认 320，对应 8kHz/25fps 的 G711）。 */
    std::size_t frame_bytes_ = 320;
};

} // namespace detail
} // namespace ipc_rtsp
