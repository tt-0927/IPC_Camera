/**
 * @FilePath     : annexb_scanner.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : Annex-B NAL 分类实现
 */

#include "media/annexb_scanner.h"

namespace ipc_rtsp
{
namespace detail
{

NalUnitType ClassifyNal(Codec codec, const std::uint8_t *nal, std::size_t size)
{
    if (nal == nullptr || size == 0)
    {
        return NalUnitType::Unknown;
    }

    if (codec == Codec::H264)
    {
        /* nal_unit_type（nal[0] 低 5 位）：1~4 为非 IDR slice（2/3/4 为
         * slice 分片），5 为 IDR slice，6 SEI，7 SPS，8 PPS，9 AUD。 */
        switch (static_cast<unsigned>(nal[0] & 0x1fu))
        {
        case 1:
        case 2:
        case 3:
        case 4:
            return NalUnitType::NonIdrSlice;
        case 5:
            return NalUnitType::IdrSlice;
        case 6:
            return NalUnitType::Sei;
        case 7:
            return NalUnitType::Sps;
        case 8:
            return NalUnitType::Pps;
        case 9:
            return NalUnitType::Aud;
        default:
            return NalUnitType::Other;
        }
    }

    if (codec == Codec::H265)
    {
        if (size < 2u)
        {
            return NalUnitType::Unknown;
        }
        const unsigned unit_type = (static_cast<unsigned>(nal[0]) >> 1u) & 0x3fu;

        /* 注意：IDR_W_RADL(19)/IDR_N_LP(20)/CRA(21) 也在 0~31 区间内，
         * 必须先判定这些随机接入类型，再回落到普通图像 NAL。 */
        switch (unit_type)
        {
        case 19:
        case 20:
            return NalUnitType::IdrSlice;
        case 21:
            return NalUnitType::CraSlice;
        case 32:
            return NalUnitType::Vps;
        case 33:
            return NalUnitType::Sps;
        case 34:
            return NalUnitType::Pps;
        case 35:
            return NalUnitType::Aud;
        case 39:
        case 40:
            return NalUnitType::Sei;
        default:
            return unit_type <= 31u ? NalUnitType::NonIdrSlice : NalUnitType::Other;
        }
    }

    return NalUnitType::Other;
}

bool IsFirstSliceOfPicture(Codec codec, const std::uint8_t *nal, std::size_t size)
{
    if (nal == nullptr || size == 0)
    {
        return false;
    }

    const NalUnitType type = ClassifyNal(codec, nal, size);
    if (!IsVclNal(type))
    {
        return false;
    }

    if (codec == Codec::H264)
    {
        /* 跳过 1 字节 NAL 头后是 slice header 的第一个 ue(v)：
         * 首位为 1 表示 first_mb_in_slice == 0（新图像）。 */
        if (size < 2u)
        {
            return false;
        }
        return (nal[1] & 0x80u) != 0u;
    }

    if (codec == Codec::H265)
    {
        /* 跳过 2 字节 NAL 头后是 first_slice_segment_in_pic_flag（1 bit）。 */
        if (size < 3u)
        {
            return false;
        }
        return (nal[2] & 0x80u) != 0u;
    }

    return false;
}

} // namespace detail
} // namespace ipc_rtsp
