/**
 * @FilePath     : annexb_scanner.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : Annex-B 码流扫描与 NAL 分类
 */

/*
 * 统一处理 3/4 字节起始码、复合 pack（单包模式下一个 buffer 含
 * SPS/PPS/SEI/IDR）与多包模式，供首帧策略、关键帧判定与 RTP 打包共用。
 */
#pragma once

#include "ipc_rtsp/types.h"

#include <cstddef>
#include <cstdint>

namespace ipc_rtsp
{
namespace detail
{

/** 一个 NAL 在 buffer 中的范围（不含起始码）。 */
struct NalRange
{
    /* NAL 净荷起始（memory: 非拥有，指向调用方传入的 buffer 内部，
     * 生命周期与该 buffer 一致）。 */
    const std::uint8_t *data = nullptr;
    /* NAL 净荷字节数（字节，不含起始码）。 */
    std::size_t size = 0;
    /* NAL 类型（按 codec 分类）。 */
    NalUnitType type = NalUnitType::Unknown;
};

/** 是否为参数集（VPS/SPS/PPS）。@param type NAL 类型。@return 是参数集返回 true。 */
inline bool IsParameterSet(NalUnitType type)
{
    return type == NalUnitType::Vps || type == NalUnitType::Sps || type == NalUnitType::Pps;
}

/** 是否可作为解码随机接入点（IDR / CRA / MJPEG 独立帧由调用方补充判断）。
 *  @param type NAL 类型。@return IDR/CRA slice 返回 true。 */
inline bool IsKeyNal(NalUnitType type)
{
    return type == NalUnitType::IdrSlice || type == NalUnitType::CraSlice;
}

/** 是否为 VCL（图像）NAL。@param type NAL 类型。@return 非 IDR/IDR/CRA
 *  slice 之一返回 true。 */
inline bool IsVclNal(NalUnitType type)
{
    return type == NalUnitType::NonIdrSlice || type == NalUnitType::IdrSlice || type == NalUnitType::CraSlice;
}

/**
 * 判断一个 VCL NAL 是否是**新图像的第一个 slice**。
 *
 * 用于把多 slice 编码（如 x264 sliced-threads 每帧 5 个 slice）正确归入同一
 * Access Unit。判定依据编码语法：
 * - H.264：slice header 首个 ue(v) `first_mb_in_slice == 0`；
 * - H.265：slice segment header 首比特 `first_slice_segment_in_pic_flag == 1`。
 *
 * @param codec 码流编码类型
 * @param nal NAL 净荷起始（须含 NAL 头与判定所需的首个语法元素字节）
 * @param size NAL 净荷字节数
 * @return 是新图像首片返回 true；非 VCL NAL 或数据不足返回 false
 */
bool IsFirstSliceOfPicture(Codec codec, const std::uint8_t *nal, std::size_t size);

/**
 * 按 codec 分类 NAL。
 *
 * @param codec 码流编码类型
 * @param nal NAL 净荷起始；至少 1 字节（H.265 需要 2 字节）
 * @param size NAL 净荷字节数
 * @return NAL 类型；无法解析时返回 Unknown，未映射类型返回 Other
 */
NalUnitType ClassifyNal(Codec codec, const std::uint8_t *nal, std::size_t size);

/**
 * 遍历 buffer 中的所有 NAL。
 *
 * @param codec 码流编码类型（用于 NAL 分类）
 * @param data Annex-B 码流起始
 * @param size 码流字节数（字节）
 * @param fn 回调签名 `bool(const NalRange&)`；返回 false 提前结束
 * @return 实际遍历到的 NAL 数量
 */
template <typename Fn>
std::size_t ForEachNal(Codec codec, const std::uint8_t *data, std::size_t size, Fn &&fn)
{
    std::size_t count = 0;

    auto find_start_code = [&](std::size_t from, std::size_t &start, std::size_t &code_len) -> bool
    {
        for (std::size_t i = from; i + 2u < size; ++i)
        {
            if (data[i] != 0 || data[i + 1u] != 0)
            {
                continue;
            }
            if (data[i + 2u] == 1u)
            {
                /* 00 00 01；若前一个字节也是 0，则属于四字节起始码尾部。 */
                if (i > 0 && data[i - 1u] == 0)
                {
                    start = i - 1u;
                    code_len = 4u;
                }
                else
                {
                    start = i;
                    code_len = 3u;
                }
                return true;
            }
        }
        return false;
    };

    std::size_t au_start = 0;
    std::size_t code_len = 0;
    if (!find_start_code(0, au_start, code_len))
    {
        return 0;
    }

    std::size_t cursor = au_start + code_len;
    for (;;)
    {
        std::size_t next_start = 0;
        std::size_t next_code_len = 0;
        const bool has_next = find_start_code(cursor, next_start, next_code_len);

        std::size_t nal_end = has_next ? next_start : size;
        /* 去掉 Annex-B 的 trailing_zero_8bits（起始码前的填充零）。 */
        while (nal_end > cursor && data[nal_end - 1u] == 0)
        {
            --nal_end;
        }

        if (nal_end > cursor)
        {
            NalRange range;
            range.data = data + cursor;
            range.size = nal_end - cursor;
            range.type = ClassifyNal(codec, range.data, range.size);
            ++count;
            if (!fn(range))
            {
                return count;
            }
        }

        if (!has_next)
        {
            break;
        }
        cursor = next_start + next_code_len;
    }

    return count;
}

} // namespace detail
} // namespace ipc_rtsp
