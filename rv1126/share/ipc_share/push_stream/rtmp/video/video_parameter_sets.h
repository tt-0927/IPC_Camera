/**
 * @FilePath     : video_parameter_sets.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-13 08:55:30
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-13 09:39:05
 * @Description  : RTMP视频参数集定义
 */

#pragma once

#include <cstdint>
#include <vector>

/**
 * @brief RTMP视频码流处理命名空间
 */
namespace RtmpVideo_NS
{
    /**
     * @brief 视频参数集缓存
     */
    struct VideoParameterSets_S
    {
        /* H.265 VPS参数集 */
        std::vector<uint8_t> vps;
        /* H.264/H.265 SPS参数集 */
        std::vector<uint8_t> sps;
        /* H.264/H.265 PPS参数集 */
        std::vector<uint8_t> pps;
    };
}
