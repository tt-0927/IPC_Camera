/**
 * @FilePath     : aac_extradata_builder.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-13 08:55:30
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-13 09:35:05
 * @Description  : AAC extradata构造工具实现
 */

#include "aac_extradata_builder.h"

namespace RtmpAudio_NS
{
    bool build_aac_extradata(const AacAdtsInfo_S& stInfo, std::vector<uint8_t>& vExtradata)
    {
        if (stInfo.object_type <= 0 || stInfo.sample_rate_index < 0 ||
            stInfo.sample_rate_index > 12 || stInfo.channels <= 0)
        {
            return false;
        }

        vExtradata.clear();
        vExtradata.push_back(static_cast<uint8_t>((stInfo.object_type << 3) |
                                                  ((stInfo.sample_rate_index >> 1) & 0x07)));
        vExtradata.push_back(static_cast<uint8_t>(((stInfo.sample_rate_index & 0x01) << 7) |
                                                  (stInfo.channels << 3)));
        return true;
    }
}
