/**
 * @FilePath     : aac_extradata_builder.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-13 08:55:30
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-13 09:35:27
 * @Description  : AAC extradata构造工具
 */

#pragma once

#include <cstdint>
#include <vector>

#include "aac_adts_parser.h"

namespace RtmpAudio_NS
{
    /**
     * @brief 构造AAC AudioSpecificConfig
     * @param stInfo ADTS解析信息
     * @param vExtradata 输出extradata
     * @return true：构造成功，false：参数无效
     */
    bool build_aac_extradata(const AacAdtsInfo_S& stInfo, std::vector<uint8_t>& vExtradata);
}
