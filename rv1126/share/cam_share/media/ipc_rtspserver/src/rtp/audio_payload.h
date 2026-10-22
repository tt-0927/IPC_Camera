/**
 * @FilePath     : audio_payload.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-14 14:38:54
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 音频 RTP 负载构造（按编码格式的统一入口）
 */

/*
 * 各格式的打包规则集中在这一处，发送循环只调用 BuildAudioPayload：
 * - AAC：RFC 3640 mpeg4-generic（SDP 声明 AAC-hbr; sizelength=13; indexlength=3），
 *   单帧打包为 [AU-headers-length=16bit][AU header: size<<3|index][裸帧]；
 * - G711A/G711U/G726：RFC 3551 整帧透传，无额外封装。
 */
#pragma once

#include "ipc_rtsp/types.h"

#include <cstddef>
#include <cstdint>

namespace ipc_rtsp
{
namespace detail
{

/** AAC AU 头段固定 4 字节（2 字节长度 + 2 字节单个 AU header）。 */
constexpr std::size_t kAacAuPrefixBytes = 4;
/** RFC 3640 sizelength=13 的单帧上限。 */
constexpr std::size_t kAacAuMaxFrameBytes = 8191;
/** 音频帧槽位上限（与邮箱槽一致），输出缓冲按"前缀 + 槽位"准备即可。 */
constexpr std::size_t kAudioFrameSlotBytes = 4096;

/**
 * @brief 构造音频 RTP 负载。
 *
 * @param codec 音频编码格式（决定打包方式）
 * @param data 输入帧数据（裸 AU；调用方须已按配置剥离 ADTS 头）
 * @param size 输入帧字节数
 * @param out 输出缓冲，容量须 >= kAacAuPrefixBytes + size
 * @param cap 输出缓冲容量（字节）
 * @return 负载字节数；0 表示参数非法或 AAC 帧超 13 位上限（调用方丢弃计数）。
 */
std::size_t BuildAudioPayload(AudioCodec codec, const std::uint8_t *data, std::size_t size, std::uint8_t *out, std::size_t cap);

} // namespace detail
} // namespace ipc_rtsp
