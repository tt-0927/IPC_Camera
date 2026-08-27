/**
 * @FilePath     : annexb_nal_parser.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-13 08:55:30
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-13 09:38:23
 * @Description  : AnnexB NAL解析工具
 */

#pragma once

#include <cstdint>

#include "video_parameter_sets.h"
#include "video_define.h"

namespace RtmpVideo_NS
{
    /**
     * @brief 从AnnexB H.264帧中提取SPS/PPS
     * @param pData 帧数据
     * @param nSize 帧长度
     * @param stSets 输出参数集
     * @return true：提取成功，false：缺少SPS或PPS
     */
    bool extract_h264_parameter_sets(const uint8_t* pData, int nSize, VideoParameterSets_S& stSets);

    /**
     * @brief 从AnnexB H.265帧中提取VPS/SPS/PPS
     * @param pData 帧数据
     * @param nSize 帧长度
     * @param stSets 输出参数集
     * @return true：提取成功，false：缺少VPS/SPS/PPS
     */
    bool extract_h265_parameter_sets(const uint8_t* pData, int nSize, VideoParameterSets_S& stSets);

    /**
     * @brief 解析帧数据查找关键帧NAL单元
     * @param pData 帧数据
     * @param nSize 帧长度
     * @param enCodec 视频编码类型
     * @return true：包含关键帧NAL，false：不包含
     */
    bool has_key_frame_nal(const uint8_t* pData, int nSize, Video_NS::VideoCodec_E enCodec);
}
