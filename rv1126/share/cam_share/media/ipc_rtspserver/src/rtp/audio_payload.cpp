/**
 * @FilePath     : audio_payload.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-14 14:38:54
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 音频 RTP 负载构造实现
 */

#include "rtp/audio_payload.h"

#include <cstring>

namespace ipc_rtsp
{
namespace detail
{

std::size_t BuildAudioPayload(AudioCodec codec, const std::uint8_t *data, std::size_t size, std::uint8_t *out, std::size_t cap)
{
    if (data == nullptr || out == nullptr || size == 0)
    {
        return 0;
    }

    if (codec != AudioCodec::AAC)
    {
        /* G711/G726 等整帧透传格式。 */
        if (cap < size)
        {
            return 0;
        }
        std::memcpy(out, data, size);
        return size;
    }

    if (size > kAacAuMaxFrameBytes || cap < kAacAuPrefixBytes + size)
    {
        return 0;
    }

    /* AU-headers-length = 16 bit（一个 16 位 AU header）；header 高 13 位为帧长。 */
    out[0] = 0x00;
    out[1] = 0x10;
    out[2] = static_cast<std::uint8_t>(size >> 5);
    out[3] = static_cast<std::uint8_t>((size << 3) & 0xF8u);
    std::memcpy(out + kAacAuPrefixBytes, data, size);
    return kAacAuPrefixBytes + size;
}

} // namespace detail
} // namespace ipc_rtsp
