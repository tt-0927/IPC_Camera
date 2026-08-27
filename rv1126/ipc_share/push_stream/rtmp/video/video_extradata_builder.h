/**
 * @FilePath     : video_extradata_builder.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-13 08:55:30
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-13 09:38:48
 * @Description  : RTMP视频extradata构造工具
 */

#pragma once

#include <cstdint>
#include <vector>

#include "video_parameter_sets.h"

namespace RtmpVideo_NS
{
    /**
     * @brief 构造H.264 AVCDecoderConfigurationRecord
     * @param stSets H.264 SPS/PPS参数集
     * @param vExtradata 输出extradata
     * @return true：构造成功，false：参数集无效
     */
    bool build_avc_extradata(const VideoParameterSets_S& stSets, std::vector<uint8_t>& vExtradata);

    /**
     * @brief 构造H.265 HEVCDecoderConfigurationRecord
     * @param stSets H.265 VPS/SPS/PPS参数集
     * @param vExtradata 输出extradata
     * @return true：构造成功，false：参数集无效
     */
    bool build_hevc_extradata(const VideoParameterSets_S& stSets, std::vector<uint8_t>& vExtradata);
}
