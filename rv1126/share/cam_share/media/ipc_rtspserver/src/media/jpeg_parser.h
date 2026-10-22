/**
 * @FilePath     : jpeg_parser.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-14 15:21:35
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : JPEG 帧解析（RFC 2435 打包前置）
 */

/*
 * 从 VENC 产出的完整 JPEG 帧（SOI...EOI）中提取 RFC 2435 主头所需的
 * type/宽高块数/量化表/重启间隔与熵编码数据区间。只做零拷贝定位：
 * 输出指针指向输入帧内部，仅在当次发送期间有效。
 */
#pragma once

#include "ipc_rtsp/result.h"

#include <cstddef>
#include <cstdint>

namespace ipc_rtsp
{
namespace detail
{

/** 单张 8bit baseline 量化表的字节数（RFC 2435 与 JPEG DQT 一致）。 */
constexpr std::size_t kJpegQuantTableBytes = 64;

/** RFC 2435 打包所需的帧描述（全部为帧内零拷贝视图）。 */
struct JpegFrameInfo
{
    /* 0=4:2:2、1=4:2:0；采样无法识别时回退默认 1；带重启标记时为基值+64。 */
    std::uint8_t type = 0;
    /* ceil(width/8)；像素超过 2040 时置 0（接收端尺寸未知）。 */
    std::uint8_t width_blocks = 0;
    /* ceil(height/8)；同上。 */
    std::uint8_t height_blocks = 0;
    /* DRI 值；0 表示无重启标记。 */
    std::uint16_t restart_interval = 0;
    /* 量化表（zigzag 序）；亮度表必须存在。memory: 非拥有，指向输入帧内部。 */
    const std::uint8_t *qt[2] = { nullptr, nullptr };
    /* SOS 段之后、EOI 之前的熵编码数据。memory: 非拥有，指向输入帧内部。 */
    const std::uint8_t *scan = nullptr;
    /* 熵编码数据字节数（字节）。 */
    std::size_t scan_size = 0;
};

/**
 * 解析失败阶段（板端日志定位用）。接受面与旧 live555 路径对齐后，
 * 能命中的都是帧结构异常，不再有"采样/分量数超出 RFC 2435"一类能力性拒绝。
 */
enum class JpegRejectStage : std::uint8_t
{
    /* 无失败。 */
    kNone = 0,
    /* 非 SOI 开头或长度不足。 */
    kBadStart,
    /* 段长非法或越界（value_a=marker、value_b=段长）。 */
    kBadSegment,
    /* 缺 SOF0/SOS/亮度量化表（value_a 位图：bit0=SOF0、bit1=SOS、bit2=qt0）。 */
    kIncomplete,
    /* DQT 16bit 表（Pq=1），打包器只支持 8bit 表（value_a=表号）。 */
    kDqtPrecision,
    /* 熵编码数据超过 24bit fragment_offset 可表示范围。 */
    kScanTooLarge,
};

/** 解析失败的附加定位信息；允许传 nullptr 表示不关心。 */
struct JpegParseDiag
{
    /* 失败阶段。 */
    JpegRejectStage stage = JpegRejectStage::kNone;
    /* 阶段相关附加值 A（含义见各阶段说明）。 */
    unsigned value_a = 0;
    /* 阶段相关附加值 B。 */
    unsigned value_b = 0;
};

/** 返回失败阶段的短码（日志用），如 "segment"、"incomplete"。 */
const char *JpegRejectStageToString(JpegRejectStage stage);

/**
 * 解析一帧完整 JPEG。
 *
 * 接受面对齐旧 live555 路径（mjpeg_video_source）：SOF 字段尽力解析，
 * 采样判不出 type 时回退默认 1 照发，不检查分量数与色度采样；像素超过
 * 2040 时宽高块数置 0（与旧路径行为一致）。仅 16bit 量化表（打包器不
 * 支持）与结构性损坏仍拒绝。
 *
 * @param data JPEG 帧起始（SOI 开头，仅当次调用期间有效）
 * @param size 帧总字节数（字节）
 * @param[out] out 帧描述（帧内零拷贝视图）
 * @param[out] diag 可选，失败时填入定位信息；传 nullptr 忽略
 *
 * @return Ok；InvalidArgument 表示数据损坏（marker 越界/缺 SOF0/SOS/量化表）；
 *         Unsupported 表示格式超出打包器能力
 */
Result ParseJpegFrame(const std::uint8_t *data, std::size_t size, JpegFrameInfo *out, JpegParseDiag *diag = nullptr);

} // namespace detail
} // namespace ipc_rtsp
