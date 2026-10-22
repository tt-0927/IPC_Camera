/**
 * @FilePath     : sdp.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : SDP 生成实现
 */

#include "rtsp/sdp.h"

#include "support/text.h"

#include <cstdio>
#include <cstring>

namespace ipc_rtsp
{
namespace detail
{
namespace
{

/**
 * @brief AAC 采样率 → AudioSpecificConfig 的 samplingFrequencyIndex。
 *
 * @param sample_rate AAC 采样率（Hz）
 * @return RFC 3640 表中的频率索引；不支持的采样率返回 -1
 */
int AacSampleRateIndex(std::uint32_t sample_rate)
{
    switch (sample_rate)
    {
    case 96000:
        return 0;
    case 88200:
        return 1;
    case 64000:
        return 2;
    case 48000:
        return 3;
    case 44100:
        return 4;
    case 32000:
        return 5;
    case 24000:
        return 6;
    case 22050:
        return 7;
    case 16000:
        return 8;
    case 12000:
        return 9;
    case 11025:
        return 10;
    case 8000:
        return 11;
    default:
        return -1;
    }
}

/**
 * @brief 生成 AAC AudioSpecificConfig（2 字节）的十六进制字符串。
 *
 * @param sample_rate AAC 采样率（Hz）
 * @param channels 声道数；非 1~7 时按单声道处理
 * @return 4 字符十六进制串；采样率不支持返回空串
 */
std::string AacConfigHex(std::uint32_t sample_rate, int channels)
{
    const int freq_index = AacSampleRateIndex(sample_rate);
    if (freq_index < 0)
    {
        return std::string();
    }
    const int channel_config = (channels <= 0 || channels > 7) ? 1 : channels;
    const std::uint8_t bytes[2] = {
        static_cast<std::uint8_t>((2u << 3u) | (static_cast<unsigned>(freq_index) >> 1u)),
        static_cast<std::uint8_t>(((static_cast<unsigned>(freq_index) & 1u) << 7u) | (static_cast<unsigned>(channel_config) << 3u)),
    };
    return HexEncode(bytes, sizeof bytes);
}

/**
 * @brief 追加一行 "a=" 属性。
 *
 * @param out 输出 SDP 文本，向末尾追加
 * @param value 属性内容（不含 "a=" 前缀）
 * @return {void}
 */
void AppendAttr(std::string *out, const std::string &value)
{
    *out += "a=";
    *out += value;
    *out += "\r\n";
}

/**
 * @brief 音频媒体行的属性段：control/rtpmap/fmtp 按 codec 组合输出。
 *
 * @param out 输出 SDP 文本，向末尾追加（不含 m=audio 行）
 * @param stream 码流配置，提供音频 codec/负载类型/时钟/声道
 * @return 音频参数完整返回 true；codec 不支持或 AAC 参数集无法生成返回 false
 */
bool AppendAudio(std::string *out, const StreamConfig &stream)
{
    const AudioCodec codec = stream.audio_codec;
    const std::uint8_t pt = stream.audio_payload_type;

    char line[192];
    switch (codec)
    {
    case AudioCodec::G711A:
        AppendAttr(out, "control:trackID=1");
        snprintf(line, sizeof line, "rtpmap:%u PCMA/%u", pt, stream.audio_clock_rate);
        AppendAttr(out, line);
        return true;
    case AudioCodec::G711U:
        AppendAttr(out, "control:trackID=1");
        snprintf(line, sizeof line, "rtpmap:%u PCMU/%u", pt, stream.audio_clock_rate);
        AppendAttr(out, line);
        return true;
    case AudioCodec::G726_32:
        AppendAttr(out, "control:trackID=1");
        snprintf(line, sizeof line, "rtpmap:%u G726-32/%u", pt, stream.audio_clock_rate);
        AppendAttr(out, line);
        return true;
    case AudioCodec::AAC:
    {
        const std::string config = AacConfigHex(stream.audio_clock_rate, stream.audio_channels);
        if (config.empty())
        {
            return false;
        }
        AppendAttr(out, "control:trackID=1");
        snprintf(line,
                 sizeof line,
                 "rtpmap:%u MPEG4-GENERIC/%u/%d",
                 pt,
                 stream.audio_clock_rate,
                 stream.audio_channels <= 0 ? 1 : stream.audio_channels);
        AppendAttr(out, line);
        snprintf(line,
                 sizeof line,
                 "fmtp:%u streamtype=5;profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;config=%s",
                 pt,
                 config.c_str());
        AppendAttr(out, line);
        return true;
    }
    default:
        return false;
    }
}

} // namespace

bool BuildStreamSdp(const SdpInput &input, std::string *out)
{
    if (out == nullptr || input.stream == nullptr || input.codec_config == nullptr)
    {
        return false;
    }

    const StreamConfig &stream = *input.stream;
    const CodecConfigCache &config = *input.codec_config;
    if (!config.Ready())
    {
        return false;
    }

    /* 用指针而非 const 引用：条件表达式两个分支的值类别不一致时可能生成临时对象
     * （cppcheck 也会报 danglingTemporaryLifetime），指针形式语义明确且无拷贝。 */
    static const std::string kFallbackIp("0.0.0.0");
    const std::string *ip = input.advertised_ip.empty() ? &kFallbackIp : &input.advertised_ip;

    char line[256];
    out->clear();
    out->reserve(768);

    *out += "v=0\r\n";
    snprintf(line, sizeof line, "o=- %u 1 IN IP4 %s\r\n", input.session_id, ip->c_str());
    *out += line;
    *out += "s=";
    *out += input.server_name;
    *out += "\r\n";
    snprintf(line, sizeof line, "c=IN IP4 %s\r\n", ip->c_str());
    *out += line;
    *out += "t=0 0\r\n";

    /* 视频媒体行：端口固定 0（UDP 端口在 SETUP 中协商）。 */
    snprintf(line, sizeof line, "m=video 0 RTP/AVP %u\r\n", stream.payload_type);
    *out += line;
    AppendAttr(out, "control:trackID=0");
    if (stream.codec == Codec::H264)
    {
        snprintf(line, sizeof line, "rtpmap:%u H264/%u", stream.payload_type, stream.clock_rate);
        AppendAttr(out, line);
        /* sprop 为 base64(SPS)+","+base64(PPS)，参数集上限各 512B 时可达 ~1.4KB，
         * 固定栈缓冲会静默截断导致客户端解析失败，与 H265 分支同口径用 string 拼接。 */
        const std::string sprop = config.H264SpropParameterSets();
        std::string fmtp;
        fmtp.reserve(96 + sprop.size());
        snprintf(line, sizeof line, "fmtp:%u packetization-mode=1;profile-level-id=", stream.payload_type);
        fmtp += line;
        fmtp += config.H264ProfileLevelId();
        fmtp += ";sprop-parameter-sets=";
        fmtp += sprop;
        AppendAttr(out, fmtp);
    }
    else if (stream.codec == Codec::H265)
    {
        snprintf(line, sizeof line, "rtpmap:%u H265/%u", stream.payload_type, stream.clock_rate);
        AppendAttr(out, line);
        std::string fmtp;
        fmtp.reserve(256);
        snprintf(line, sizeof line, "fmtp:%u ", stream.payload_type);
        fmtp += line;
        const std::string vps = config.H265SpropVps();
        if (!vps.empty())
        {
            fmtp += "sprop-vps=";
            fmtp += vps;
            fmtp += ";";
        }
        fmtp += "sprop-sps=";
        fmtp += config.H265SpropSps();
        fmtp += ";sprop-pps=";
        fmtp += config.H265SpropPps();
        AppendAttr(out, fmtp);
    }
    else if (stream.codec == Codec::MJPEG)
    {
        /* MJPEG 使用静态负载类型 26（JPEG/90000）：静态 PT 无需 rtpmap 行，
         * 与旧 live555 服务器的 SDP 输出一致（payload_type 由服务层归一化为 26）。 */
    }
    else
    {
        /* 未知/未特判的 codec：仍按动态负载类型生成 rtpmap，名称取 CodecToString，
         * 保证 SDP 结构完整（客户端据此可识别无法解码的 payload）。 */
        snprintf(line, sizeof line, "rtpmap:%u %s/%u", stream.payload_type, CodecToString(stream.codec), stream.clock_rate);
        AppendAttr(out, line);
    }

    if (stream.fps > 0)
    {
        snprintf(line, sizeof line, "framerate:%d", stream.fps);
        AppendAttr(out, line);
    }
    AppendAttr(out, "sendonly");

    if (input.include_audio && stream.audio_enabled && stream.audio_codec != AudioCodec::None)
    {
        snprintf(line, sizeof line, "m=audio 0 RTP/AVP %u\r\n", stream.audio_payload_type);
        *out += line;
        if (!AppendAudio(out, stream))
        {
            /* 音频参数非法时只保留视频轨，避免生成不可用 SDP。 */
        }
        else
        {
            AppendAttr(out, "sendonly");
        }
    }

    return true;
}

} // namespace detail
} // namespace ipc_rtsp
