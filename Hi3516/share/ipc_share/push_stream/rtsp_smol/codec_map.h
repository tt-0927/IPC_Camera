/**
 * @FilePath     : codec_map.h
 * @Author       : zhouzr@kfb.cn
 * @Description  : 产品枚举 -> ipc_rtsp 库枚举映射（纯函数，无状态）
 *
 * 收纳 rtsp_smol 薄封装中的全部"翻译"逻辑：
 *   - 视频/音频编码枚举映射
 *   - NAL 类型映射（H264/H265 两套）
 *   - RTP 动态负载类型分配
 *   - 主码流按码率分档的连接上限策略
 *
 * 新增编码格式或调整 PT 分配只改本模块；类实现不依赖这里的细节。
 */
#pragma once

#include <cstdint>

#include "audio_define.h"
#include "video_define.h"

#include "ipc_rtsp/types.h"

namespace rtsp_smol
{

/* RTP 动态负载类型分配：视频 96，AAC/G.726 用 97/98，G.711 用静态 PT。 */
constexpr uint8_t kVideoPayloadType = 96;

/** 把产品视频编码枚举映射为库枚举。 */
ipc_rtsp::Codec to_codec(Video_NS::VideoCodec_E codec);

/** 把产品音频格式枚举映射为库枚举。 */
ipc_rtsp::AudioCodec to_audio_codec(Audio_NS::AudioFormat_E format);

/** 音频负载类型。 */
uint8_t audio_payload_type(ipc_rtsp::AudioCodec codec);

/** 把产品的首个 NAL 类型映射为库的 NAL 分类（用于首帧策略与关键帧判定）。 */
ipc_rtsp::NalUnitType to_nal_type(Video_NS::VideoCodec_E codec, Video_NS::NalType_E type);

/** 主码流按码率分档的连接上限（与 live555 实现保持一致的策略）。 */
int get_main_client_limit(int bitrate_kbps);

} // namespace rtsp_smol
