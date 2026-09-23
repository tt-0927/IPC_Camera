/**
 * @FilePath     : jpeg_file_source.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-14 15:21:46
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 主机验证用的 Motion-JPEG 文件媒体源
 */

/*
 * 仅用于 PoC 与自动化测试：把 mjpeg 复用流（连续的 SOI...EOI 帧）切成
 * 单帧后按帧率推送。板端 VENC 直接产出完整 JPEG 帧，无需此切分。
 */
#pragma once

#include "ipc_rtsp/frame.h"
#include "ipc_rtsp/result.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ipc_rtsp
{
namespace detail
{

/** Motion-JPEG 复用文件源（按 SOI/EOI 边界切帧）。 */
class JpegFileSource
{
public:
    /**
     * @brief 读取文件并按 SOI/EOI 切帧；失败返回错误。
     * @param path mjpeg 复用流文件路径
     * @return 成功返回 Ok；打不开/空文件/无完整帧返回对应错误
     */
    Result Load(const std::string &path);

    /**
     * 取下一帧。
     *
     * @param[out] out 帧视图（指向内部缓冲，Load 之后一直有效）
     * @return 到达末尾且未循环时返回 false
     */
    bool NextAu(VideoFrameView *out);

    /** 设置是否循环播放（到末尾时回绕重播）。 */
    void set_loop(bool loop)
    {
        loop_ = loop;
    }

    /** @brief 切分出的 JPEG 帧数。@return 帧数（帧）。 */
    std::size_t frame_count() const
    {
        return frames_.size();
    }

private:
    /* 单帧在文件缓冲中的字节区间。 */
    struct FrameRange
    {
        /* 帧首（SOI 的 0xFF）在 data_ 中的偏移。 */
        std::size_t offset = 0;
        /* 帧总字节数（字节，SOI...EOI 含 EOI 两字节）。 */
        std::size_t size = 0;
    };

    /* 整个文件内容（memory: 拥有；NextAu 输出视图指向其内部，Load 后
     * 未再 Load 前一直有效）。 */
    std::vector<std::uint8_t> data_;
    /* 切分出的帧区间表（Load 时填充）。 */
    std::vector<FrameRange> frames_;
    /* 下一个待取帧下标；到末尾时按 loop_ 决定回绕或停止。 */
    std::size_t cursor_ = 0;
    /* 到末尾是否回绕重播。 */
    bool loop_ = true;
};

} // namespace detail
} // namespace ipc_rtsp
