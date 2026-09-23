/**
 * @FilePath     : test_audio_payload.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-14 14:38:54
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 音频 RTP 负载构造测试
 */

#include "rtp/audio_payload.h"

#include "test_support.h"

#include <array>
#include <cstring>

using namespace ipc_rtsp;
using namespace ipc_rtsp::detail;

RTSP_TEST_CASE(aac_payload_prepends_au_header)
{
    /* 场景：AAC 裸帧打包；预期在帧前插入 4 字节 AU 头（2 字节长度字段 + 2 字节 AU header）。 */
    const std::array<std::uint8_t, 300> frame = { 0xAB, 0xCD };
    std::array<std::uint8_t, kAacAuPrefixBytes + 4096> out{};

    const std::size_t size = BuildAudioPayload(AudioCodec::AAC, frame.data(), frame.size(), out.data(), out.size());
    RTSP_CHECK_EQ(size, kAacAuPrefixBytes + frame.size());
    /* AU-headers-length = 16 bit。 */
    RTSP_CHECK_EQ(out[0], 0);
    RTSP_CHECK_EQ(out[1], 0x10);
    /* AU header：高 13 位帧长（300 = 0x12C → 0x12C<<3 = 0x0960）。 */
    RTSP_CHECK_EQ(out[2], 0x09);
    RTSP_CHECK_EQ(out[3], 0x60);
    RTSP_CHECK(std::memcmp(out.data() + kAacAuPrefixBytes, frame.data(), frame.size()) == 0);

    /* 帧长编码在 13 位内可表示（4096B 帧：0x1000<<3 = 0x8000）。 */
    const std::array<std::uint8_t, 4096> big = {};
    RTSP_CHECK_EQ(BuildAudioPayload(AudioCodec::AAC, big.data(), big.size(), out.data(), out.size()), kAacAuPrefixBytes + big.size());
    RTSP_CHECK_EQ(out[2], 0x80);
    RTSP_CHECK_EQ(out[3], 0x00);
}

RTSP_TEST_CASE(aac_payload_rejects_oversize_and_bad_args)
{
    /* 场景：超长帧与非法入参；预期一律拒绝并返回 0（调用方按丢弃计数）。 */
    std::array<std::uint8_t, kAacAuPrefixBytes + 4096> out{};
    /* 13 位上限 8191：超出必须拒绝（返回 0，调用方丢弃计数）。 */
    RTSP_CHECK_EQ(BuildAudioPayload(AudioCodec::AAC, out.data(), kAacAuMaxFrameBytes + 1, out.data(), out.size()), 0);
    RTSP_CHECK_EQ(BuildAudioPayload(AudioCodec::AAC, nullptr, 100, out.data(), out.size()), 0);
    RTSP_CHECK_EQ(BuildAudioPayload(AudioCodec::AAC, out.data(), 0, out.data(), out.size()), 0);
    /* 输出缓冲不足必须拒绝。 */
    RTSP_CHECK_EQ(BuildAudioPayload(AudioCodec::AAC, out.data(), 100, out.data(), kAacAuPrefixBytes), 0);
}

RTSP_TEST_CASE(g711_g726_pass_through)
{
    /* 场景：G711A/G711U/G726-32 直通路径；预期不插 AU 头，输出与输入逐字节一致。 */
    const std::array<std::uint8_t, 320> frame = { 0x55 };
    std::array<std::uint8_t, 512> out{};

    for (const AudioCodec codec : { AudioCodec::G711A, AudioCodec::G711U, AudioCodec::G726_32 })
    {
        const std::size_t size = BuildAudioPayload(codec, frame.data(), frame.size(), out.data(), out.size());
        RTSP_CHECK_EQ(size, frame.size());
        RTSP_CHECK(std::memcmp(out.data(), frame.data(), frame.size()) == 0);
    }
}
