/**
 * @FilePath     : sdp.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : SDP 生成
 */

/*
 * SDP 必须由当前 codec 参数集实时生成（不能硬编码）：分辨率/编码切换后新客户端
 * 的 DESCRIBE 必须拿到新的 sprop 参数集。
 */
#pragma once

#include "ipc_rtsp/config.h"
#include "media/codec_config.h"

#include <cstdint>
#include <string>

namespace ipc_rtsp
{
namespace detail
{

/** SDP 生成输入。 */
struct SdpInput
{
    const StreamConfig *stream = nullptr; /**< 码流配置（含音频参数） */
    /* 编解码参数集缓存（SPS/PPS/VPS 等），非拥有；调用期间须有效且 Ready()。 */
    const CodecConfigCache *codec_config = nullptr;
    std::string advertised_ip;    /**< c= 使用地址 */
    std::uint32_t session_id = 1; /**< o= 的会话标识 */
    /* 服务名，写入 SDP 的 s= 行。 */
    std::string server_name = "IPC RTSP Server";
    /* 是否附带音频媒体行；实际输出仍要求 stream 配置 audio_enabled 且 codec 有效。 */
    bool include_audio = false;
};

/**
 * @brief 生成一路码流的 SDP。
 *
 * @param input SDP 输入（码流配置、参数集缓存等），各指针须在调用期间有效
 * @param out 输出 SDP 文本（CRLF 行尾）；调用前内容会被清空
 * @return 参数集未就绪等场景返回 false
 */
bool BuildStreamSdp(const SdpInput &input, std::string *out);

} // namespace detail
} // namespace ipc_rtsp
