/**
 * @FilePath     : codec_config.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-10 18:49:34
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-16 09:25:00
 * @Description  : 参数集缓存实现
 */

#include "media/codec_config.h"

#include "media/annexb_scanner.h"

#include "support/text.h"

#include <cstring>

namespace ipc_rtsp
{
namespace detail
{

CodecConfigCache::CodecConfigCache(Codec codec) : codec_(codec)
{
}

ParameterSet &CodecConfigCache::SlotFor(NalUnitType type)
{
    switch (type)
    {
    case NalUnitType::Vps:
        return vps_;
    case NalUnitType::Sps:
        return sps_;
    case NalUnitType::Pps:
        return pps_;
    default:
        /* 非参数集不会进入缓存，这里返回 SPS 槽位仅用于防御。 */
        return sps_;
    }
}

bool CodecConfigCache::Update(NalUnitType type, const std::uint8_t *nal, std::size_t size)
{
    if (nal == nullptr || size == 0 || size > kMaxParameterSetBytes)
    {
        return false;
    }
    if (!IsParameterSet(type))
    {
        return false;
    }

    ParameterSet &slot = SlotFor(type);
    if (slot.size == size && std::memcmp(slot.data.data(), nal, size) == 0)
    {
        return false;
    }

    std::memcpy(slot.data.data(), nal, size);
    slot.size = size;
    ++generation_;
    return true;
}

bool CodecConfigCache::Ready() const
{
    if (codec_ == Codec::H264)
    {
        return sps_.valid() && pps_.valid();
    }
    if (codec_ == Codec::H265)
    {
        /* VPS 在实际码流中不一定存在，SPS+PPS 即可生成可用 SDP。 */
        return sps_.valid() && pps_.valid();
    }
    return true;
}

std::string CodecConfigCache::H264SpropParameterSets() const
{
    if (codec_ != Codec::H264 || !sps_.valid() || !pps_.valid())
    {
        return std::string();
    }
    return Base64Encode(sps_.data.data(), sps_.size) + "," + Base64Encode(pps_.data.data(), pps_.size);
}

std::string CodecConfigCache::H265SpropVps() const
{
    if (codec_ != Codec::H265 || !vps_.valid())
    {
        return std::string();
    }
    return Base64Encode(vps_.data.data(), vps_.size);
}

std::string CodecConfigCache::H265SpropSps() const
{
    if (codec_ != Codec::H265 || !sps_.valid())
    {
        return std::string();
    }
    return Base64Encode(sps_.data.data(), sps_.size);
}

std::string CodecConfigCache::H265SpropPps() const
{
    if (codec_ != Codec::H265 || !pps_.valid())
    {
        return std::string();
    }
    return Base64Encode(pps_.data.data(), pps_.size);
}

std::string CodecConfigCache::H264ProfileLevelId() const
{
    /* profile-level-id = profile_idc + constraint_set_flags + level_idc，
     * 跳过 1 字节 NAL 头后取 3 字节（profile/compat/level）。 */
    if (codec_ != Codec::H264 || sps_.size < 4u)
    {
        return "42e01e";
    }
    return HexEncode(sps_.data.data() + 1u, 3u);
}

} // namespace detail
} // namespace ipc_rtsp
