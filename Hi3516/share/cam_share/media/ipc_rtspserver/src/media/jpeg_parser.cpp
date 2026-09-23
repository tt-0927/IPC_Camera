/**
 * @FilePath     : jpeg_parser.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-14 15:21:35
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : JPEG 帧解析实现
 */

/*
 * marker 扫描策略与 FFmpeg rtpenc_jpeg 一致：逐字节找 0xFF 前导，命中带段长
 * 的 marker 后整段跳过，扫描点不会落入段数据内部；段间只可能出现 0xFF 填充
 * 字节，逐字节扫是安全的。判据宽严对齐旧 live555 路径
 * （mediaServer/mjpeg_video_source）：SOF 字段尽力解析、判不出 type 回退默认 1、
 * 不检查分量数与色度采样、超限宽高把块数字段置 0 照发。旧路径能播的帧这里
 * 一律放行，保证替换等价。
 */

#include "media/jpeg_parser.h"

namespace ipc_rtsp
{
namespace detail
{
namespace
{

/* JPEG marker（0xFF 前导之后的段类型字节）。 */

/* baseline DCT 顺序编码，唯一接受的 SOF。 */
constexpr std::uint8_t kMarkerSof0 = 0xC0;
/* 量化表。 */
constexpr std::uint8_t kMarkerDqt = 0xDB;
/* 重启间隔。 */
constexpr std::uint8_t kMarkerDri = 0xDD;
/* 扫描头（其后即熵编码数据）。 */
constexpr std::uint8_t kMarkerSos = 0xDA;

/** kIncomplete 的 value_a 位图。 */
constexpr unsigned kIncompleteSof = 1u << 0;
constexpr unsigned kIncompleteSos = 1u << 1;
constexpr unsigned kIncompleteQt0 = 1u << 2;

/**
 * @brief 读取 16 位大端整数。
 * @param p 数据起始（至少 2 字节）
 * @return 大端解释出的 16 位值
 */
std::uint16_t ReadBe16(const std::uint8_t *p)
{
    return static_cast<std::uint16_t>((static_cast<unsigned>(p[0]) << 8u) | static_cast<unsigned>(p[1]));
}

/**
 * @brief 填写解析失败定位信息（diag 为空时忽略）。
 * @param[out] diag 输出目标，允许为 nullptr
 * @param stage 失败阶段
 * @param value_a 阶段相关附加值 A（含义见 JpegRejectStage 各成员说明）
 * @param value_b 阶段相关附加值 B
 */
void SetDiag(JpegParseDiag *diag, JpegRejectStage stage, unsigned value_a = 0, unsigned value_b = 0)
{
    if (diag != nullptr)
    {
        diag->stage = stage;
        diag->value_a = value_a;
        diag->value_b = value_b;
    }
}

} // namespace

const char *JpegRejectStageToString(JpegRejectStage stage)
{
    switch (stage)
    {
    case JpegRejectStage::kBadStart:
        return "bad-start";
    case JpegRejectStage::kBadSegment:
        return "bad-segment";
    case JpegRejectStage::kIncomplete:
        return "incomplete";
    case JpegRejectStage::kDqtPrecision:
        return "dqt-16bit";
    case JpegRejectStage::kScanTooLarge:
        return "scan-too-large";
    case JpegRejectStage::kNone:
        break;
    }
    return "none";
}

Result ParseJpegFrame(const std::uint8_t *data, std::size_t size, JpegFrameInfo *out, JpegParseDiag *diag)
{
    if (diag != nullptr)
    {
        *diag = JpegParseDiag{};
    }
    if (data == nullptr || out == nullptr || size < 4)
    {
        SetDiag(diag, JpegRejectStage::kBadStart);
        return Result::Fail(Status::InvalidArgument);
    }
    if (data[0] != 0xFF || data[1] != 0xD8)
    {
        SetDiag(diag, JpegRejectStage::kBadStart);
        return Result::Fail(Status::InvalidArgument);
    }

    out->qt[0] = nullptr;
    out->qt[1] = nullptr;
    out->scan = nullptr;
    out->scan_size = 0;

    /* 熵数据内 0xFF 必跟 0x00 转义或 RSTn，因此末尾的 FFD9 就是 EOI。 */
    std::size_t end = size;
    if (data[end - 2] == 0xFF && data[end - 1] == 0xD9)
    {
        end -= 2;
    }

    bool sof_seen = false;
    bool sos_seen = false;
    unsigned width = 0;
    unsigned height = 0;
    unsigned luma_sampling = 0; /* SOF0 亮度分量的 (h<<4)|v 字节；0 表示未能读取 */
    std::uint16_t restart_interval = 0;

    std::size_t i = 2;
    while (i + 4 <= end)
    {
        if (data[i] != 0xFF)
        {
            ++i;
            continue;
        }
        const std::uint8_t marker = data[i + 1];
        if (marker == 0x00 || marker == 0x01 || (marker >= 0xD0 && marker <= 0xD7))
        {
            /* 字节转义/TEM/RSTn：无段长，跳过整个 marker。 */
            i += 2;
            continue;
        }
        if (marker == 0xFF)
        {
            /* 连续 0xFF 填充。 */
            ++i;
            continue;
        }

        /* 其余 marker 均带 16 位段长（含长度字段自身）。 */
        const std::size_t seg_len = ReadBe16(data + i + 2);
        const std::size_t seg_end = i + 2 + seg_len;
        if (seg_len < 2 || seg_end > end)
        {
            SetDiag(diag, JpegRejectStage::kBadSegment, marker, seg_len);
            return Result::Fail(Status::InvalidArgument);
        }

        switch (marker)
        {
        case kMarkerDqt:
        {
            /* 一段可含多张表，也允许分多段出现；按表号记录（仅 0/1）。 */
            std::size_t pos = i + 4;
            const std::size_t table_end = seg_end;
            while (pos + 65 <= table_end)
            {
                const unsigned pq = static_cast<unsigned>(data[pos]) >> 4u;
                const unsigned tq = static_cast<unsigned>(data[pos]) & 0x0Fu;
                if (pq != 0)
                {
                    /* 16bit 表：打包器固定 Precision=0，传出会让接收端按
                     * 64 字节/表错位解析，只能拒帧（海思/ffmpeg 均为 8bit 表）。 */
                    SetDiag(diag, JpegRejectStage::kDqtPrecision, tq);
                    return Result::Fail(Status::Unsupported);
                }
                if (tq > 1)
                {
                    /* 表号超出 RFC 2435 的两张表：该表无法映射，跳过不采信
                     * （旧路径将其写入表 1 槽位，此处直接忽略，接收端按
                     * "qt1 缺失补 qt0"的兜底逻辑拿到一致内容）。 */
                }
                else
                {
                    out->qt[tq] = data + pos + 1;
                }
                pos += 65;
            }
            break;
        }
        case kMarkerSof0:
        {
            /* 段数据：precision(1)+height(2)+width(2)+ncomp(1)+每分量3字节。
             * 段长 >= 11 才能安全读到亮度采样（与旧路径的解析门槛一致）；
             * 不足时保持默认值，后续按"判不出 type 回退默认 1"处理。 */
            if (seg_len >= 11)
            {
                height = (static_cast<unsigned>(data[i + 5]) << 8u) | data[i + 6];
                width = (static_cast<unsigned>(data[i + 7]) << 8u) | data[i + 8];
                luma_sampling = data[i + 11];
            }
            sof_seen = true;
            break;
        }
        case kMarkerDri:
        {
            /* 段长异常时整段跳过、不采信值（旧路径同款）。 */
            if (seg_len == 4)
            {
                restart_interval = ReadBe16(data + i + 4);
            }
            break;
        }
        case kMarkerSos:
        {
            out->scan = data + seg_end;
            out->scan_size = end - seg_end;
            sos_seen = true;
            break;
        }
        default:
            /* APPn/COM/DHT/SOF1/SOF2 等：整段跳过。RFC 2435 要求接收端使用
             * 标准 Huffman 表（K.3），不内联 DHT。 */
            break;
        }

        if (sos_seen)
        {
            break;
        }
        i = seg_end;
    }

    if (!sof_seen || !sos_seen || out->qt[0] == nullptr)
    {
        /* 缺 SOF0/SOS 或无亮度量化表：帧结构不完整（渐进式等非 baseline
         * 编码没有 SOF0，同样在此拒绝，与旧路径丢帧行为一致）。 */
        SetDiag(diag,
                JpegRejectStage::kIncomplete,
                (sof_seen ? kIncompleteSof : 0u) | (sos_seen ? kIncompleteSos : 0u) | (out->qt[0] != nullptr ? kIncompleteQt0 : 0u));
        return Result::Fail(Status::InvalidArgument);
    }

    /* type 判定：亮度采样 2x1=4:2:2（type 0）、2x2=4:2:0（type 1）；其余
     * 采样（灰度 1x1、4:4:4、未能读取等）RFC 2435 无法表达，与旧路径一致
     * 回退默认 type=1 照发，由接收端按 4:2:0 假设解码。 */
    std::uint8_t type = (luma_sampling == 0x21) ? 0 : 1;
    if (restart_interval > 0)
    {
        type = static_cast<std::uint8_t>(type + 64);
    }

    /* RFC 2435 宽高字段各 8bit（最大 2040 像素）：超出时块数置 0，帧照发
     * （旧路径 >2048 置 0、2040~2048 间整数溢出同样归 0，此处取连续语义）。 */
    unsigned width_blocks = (width + 7u) / 8u;
    unsigned height_blocks = (height + 7u) / 8u;
    if (width_blocks > 255)
    {
        width_blocks = 0;
    }
    if (height_blocks > 255)
    {
        height_blocks = 0;
    }

    if (out->scan_size > 0xFFFFFFu)
    {
        /* fragment offset 为 24bit，扫描数据超出可表示范围。 */
        SetDiag(diag,
                JpegRejectStage::kScanTooLarge,
                static_cast<unsigned>(out->scan_size >> 16),
                static_cast<unsigned>(out->scan_size & 0xFFFFu));
        return Result::Fail(Status::Unsupported);
    }

    out->type = type;
    out->width_blocks = static_cast<std::uint8_t>(width_blocks);
    out->height_blocks = static_cast<std::uint8_t>(height_blocks);
    out->restart_interval = restart_interval;
    return Result::Ok();
}

} // namespace detail
} // namespace ipc_rtsp
