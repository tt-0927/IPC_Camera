/**
 * @FilePath     : test_sdp.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : SDP 生成测试
 */

#include "media/codec_config.h"
#include "rtsp/sdp.h"

#include "test_support.h"

#include <cstring>
#include <vector>

using namespace ipc_rtsp;
using namespace ipc_rtsp::detail;

namespace
{

/** 构造一个 H.264 SPS/PPS 缓存。
 *
 * @return 已写入 SPS/PPS 的参数集缓存。
 */
CodecConfigCache MakeH264Config()
{
    CodecConfigCache config(Codec::H264);
    /* SPS：profile_idc=0x42, constraints=0xE0, level_idc=0x1E */
    const std::uint8_t sps[] = { 0x67, 0x42, 0xE0, 0x1E, 0xD9, 0x00, 0x50 };
    const std::uint8_t pps[] = { 0x68, 0xCE, 0x3C, 0x80 };
    config.Update(NalUnitType::Sps, sps, sizeof sps);
    config.Update(NalUnitType::Pps, pps, sizeof pps);
    return config;
}

/** 构造指定编码的主码流配置。
 *
 * @param codec 视频编码。
 * @return 预填 path/payload/clock/fps 的码流配置。
 */
StreamConfig MakeStreamConfig(Codec codec)
{
    StreamConfig config;
    config.id = StreamId::Main;
    config.path = "Streaming/Channels/101";
    config.codec = codec;
    config.payload_type = 96;
    config.clock_rate = 90000;
    config.fps = 25;
    return config;
}

} // namespace

RTSP_TEST_CASE(sdp_h264_contains_parameter_sets)
{
    const StreamConfig config = MakeStreamConfig(Codec::H264);
    const CodecConfigCache codec_config = MakeH264Config();

    SdpInput input;
    input.stream = &config;
    input.codec_config = &codec_config;
    input.advertised_ip = "192.168.1.10";
    input.session_id = 12345;
    input.server_name = "IPC RTSP Server";

    std::string sdp;
    RTSP_CHECK(BuildStreamSdp(input, &sdp));

    /* H.264 SDP：应包含会话级字段、媒体行、rtpmap/fmtp 参数集与帧率方向等完整字段。 */
    /* 会话级字段与连接信息。 */
    RTSP_CHECK(sdp.find("v=0\r\n") == 0);
    RTSP_CHECK(sdp.find("c=IN IP4 192.168.1.10\r\n") != std::string::npos);
    /* 媒体行与 track 控制。 */
    RTSP_CHECK(sdp.find("m=video 0 RTP/AVP 96\r\n") != std::string::npos);
    RTSP_CHECK(sdp.find("a=control:trackID=0\r\n") != std::string::npos);
    /* H.264 rtpmap 与 fmtp 参数集。 */
    RTSP_CHECK(sdp.find("a=rtpmap:96 H264/90000\r\n") != std::string::npos);
    RTSP_CHECK(sdp.find("packetization-mode=1") != std::string::npos);
    RTSP_CHECK(sdp.find("profile-level-id=42e01e") != std::string::npos);
    RTSP_CHECK(sdp.find("sprop-parameter-sets=") != std::string::npos);
    /* 帧率与方向；且不产出音频轨。 */
    RTSP_CHECK(sdp.find("a=framerate:25\r\n") != std::string::npos);
    RTSP_CHECK(sdp.find("a=sendonly\r\n") != std::string::npos);
    RTSP_CHECK(sdp.find("m=audio") == std::string::npos);
}

RTSP_TEST_CASE(sdp_h264_long_sprop_not_truncated)
{
    /* 回归：fmtp 行曾用 256B 栈缓冲拼接，参数集顶到上限（各 512B）时被
     * snprintf 静默截断，客户端解析 sprop 失败导致花屏/无法起播。 */
    const StreamConfig config = MakeStreamConfig(Codec::H264);
    CodecConfigCache codec_config(Codec::H264);
    std::vector<std::uint8_t> sps(512, 0x42);
    sps[0] = 0x67; /* NAL 头：type=7 (SPS) */
    std::vector<std::uint8_t> pps(64, 0x68);
    RTSP_CHECK(codec_config.Update(NalUnitType::Sps, sps.data(), sps.size()));
    RTSP_CHECK(codec_config.Update(NalUnitType::Pps, pps.data(), pps.size()));

    SdpInput input;
    input.stream = &config;
    input.codec_config = &codec_config;
    input.advertised_ip = "192.168.1.10";
    input.session_id = 12345;
    input.server_name = "IPC RTSP Server";

    std::string sdp;
    RTSP_CHECK(BuildStreamSdp(input, &sdp));

    /* SDP 必须原样包含完整 sprop（base64(512B) ≈ 684 字符），直到行尾 CRLF。 */
    const std::string expected = "a=fmtp:96 packetization-mode=1;profile-level-id=" + codec_config.H264ProfileLevelId() +
                                 ";sprop-parameter-sets=" + codec_config.H264SpropParameterSets() + "\r\n";
    RTSP_CHECK(sdp.find(expected) != std::string::npos);
}

RTSP_TEST_CASE(sdp_h265_contains_vps_sps_pps)
{
    /* 场景：H.265 参数集齐全；预期 SDP 携带 VPS/SPS/PPS 三组 sprop 与 H265 rtpmap。 */
    StreamConfig config = MakeStreamConfig(Codec::H265);
    config.payload_type = 97;

    CodecConfigCache codec_config(Codec::H265);
    const std::uint8_t vps[] = { 0x40, 0x01, 0x0C, 0x01 };
    const std::uint8_t sps[] = { 0x42, 0x01, 0x01, 0x01, 0x60 };
    const std::uint8_t pps[] = { 0x44, 0x01, 0xC0 };
    codec_config.Update(NalUnitType::Vps, vps, sizeof vps);
    codec_config.Update(NalUnitType::Sps, sps, sizeof sps);
    codec_config.Update(NalUnitType::Pps, pps, sizeof pps);

    SdpInput input;
    input.stream = &config;
    input.codec_config = &codec_config;
    input.advertised_ip = "10.0.0.2";

    std::string sdp;
    RTSP_CHECK(BuildStreamSdp(input, &sdp));
    RTSP_CHECK(sdp.find("a=rtpmap:97 H265/90000\r\n") != std::string::npos);
    RTSP_CHECK(sdp.find("sprop-vps=") != std::string::npos);
    RTSP_CHECK(sdp.find("sprop-sps=") != std::string::npos);
    RTSP_CHECK(sdp.find("sprop-pps=") != std::string::npos);
}

RTSP_TEST_CASE(sdp_mjpeg_uses_static_pt26)
{
    /* MJPEG 静态 PT 26：无 rtpmap 行，与旧 live555 服务器 SDP 输出一致。 */
    StreamConfig config = MakeStreamConfig(Codec::MJPEG);
    config.payload_type = 26; /* 服务层归一化结果 */

    CodecConfigCache codec_config(Codec::MJPEG);

    SdpInput input;
    input.stream = &config;
    input.codec_config = &codec_config;
    input.advertised_ip = "192.168.1.10";
    input.session_id = 12345;
    input.server_name = "IPC RTSP Server";

    std::string sdp;
    RTSP_CHECK(BuildStreamSdp(input, &sdp));
    RTSP_CHECK(sdp.find("m=video 0 RTP/AVP 26\r\n") != std::string::npos);
    RTSP_CHECK(sdp.find("rtpmap") == std::string::npos);
    RTSP_CHECK(sdp.find("fmtp") == std::string::npos);
    RTSP_CHECK(sdp.find("a=framerate:25\r\n") != std::string::npos);
    RTSP_CHECK(sdp.find("a=sendonly\r\n") != std::string::npos);
}

RTSP_TEST_CASE(sdp_with_g711_audio_track)
{
    /* 场景：启用 G711A 音频；预期 SDP 追加静态 PT 8 的音频媒体行、独立 trackID 与 PCMA rtpmap。 */
    StreamConfig config = MakeStreamConfig(Codec::H264);
    config.audio_enabled = true;
    config.audio_codec = AudioCodec::G711A;
    config.audio_payload_type = 8;
    config.audio_clock_rate = 8000;

    const CodecConfigCache codec_config = MakeH264Config();

    SdpInput input;
    input.stream = &config;
    input.codec_config = &codec_config;
    input.advertised_ip = "127.0.0.1";
    input.include_audio = true;

    std::string sdp;
    RTSP_CHECK(BuildStreamSdp(input, &sdp));
    RTSP_CHECK(sdp.find("m=audio 0 RTP/AVP 8\r\n") != std::string::npos);
    RTSP_CHECK(sdp.find("a=control:trackID=1\r\n") != std::string::npos);
    RTSP_CHECK(sdp.find("a=rtpmap:8 PCMA/8000\r\n") != std::string::npos);
}

RTSP_TEST_CASE(sdp_with_aac_audio_track_has_config)
{
    /* 场景：启用 AAC 音频；预期 rtpmap 用 MPEG4-GENERIC 并携带 AudioSpecificConfig。 */
    StreamConfig config = MakeStreamConfig(Codec::H264);
    config.audio_enabled = true;
    config.audio_codec = AudioCodec::AAC;
    config.audio_payload_type = 97;
    config.audio_clock_rate = 16000;
    config.audio_channels = 1;

    const CodecConfigCache codec_config = MakeH264Config();

    SdpInput input;
    input.stream = &config;
    input.codec_config = &codec_config;
    input.advertised_ip = "127.0.0.1";
    input.include_audio = true;

    std::string sdp;
    RTSP_CHECK(BuildStreamSdp(input, &sdp));
    RTSP_CHECK(sdp.find("MPEG4-GENERIC/16000/1") != std::string::npos);
    /* 16 kHz + 单声道 → AudioSpecificConfig = 0x1408。 */
    RTSP_CHECK(sdp.find("config=1408") != std::string::npos);
}

RTSP_TEST_CASE(sdp_fails_without_parameter_sets)
{
    /* 场景：参数集缓存为空；预期 SDP 生成直接失败，而不是输出缺 sprop 的残缺 SDP。 */
    const StreamConfig config = MakeStreamConfig(Codec::H264);
    CodecConfigCache codec_config(Codec::H264);

    SdpInput input;
    input.stream = &config;
    input.codec_config = &codec_config;
    input.advertised_ip = "127.0.0.1";

    std::string sdp;
    RTSP_CHECK(!BuildStreamSdp(input, &sdp));
}
