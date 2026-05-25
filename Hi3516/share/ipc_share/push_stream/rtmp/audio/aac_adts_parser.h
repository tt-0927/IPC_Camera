/**
 * @FilePath     : aac_adts_parser.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-13 08:55:30
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-13 09:34:54
 * @Description  : AAC ADTS解析工具
 */

#pragma once

#include <cstdint>

namespace RtmpAudio_NS
{
    /**
     * @brief AAC ADTS头解析结果
     */
    struct AacAdtsInfo_S
    {
        /* ADTS profile字段，0=Main，1=AAC LC */
        int profile = 0;
        /* AudioSpecificConfig中的object type */
        int object_type = 0;
        /* 采样率索引 */
        int sample_rate_index = 0;
        /* 采样率 */
        int sample_rate = 0;
        /* 声道数 */
        int channels = 0;
        /* ADTS头长度，通常为7或9字节 */
        int header_size = 0;
    };

    /**
     * @brief 解析AAC ADTS头
     * @param pData AAC ADTS帧数据
     * @param nSize 帧长度
     * @param stInfo 输出ADTS信息
     * @return true：解析成功，false：非ADTS或数据不足
     */
    bool parse_adts_header(const uint8_t* pData, int nSize, AacAdtsInfo_S& stInfo);

    /**
     * @brief 根据采样率值查找对应的采样率索引
     * @param nSampleRate 采样率（如 16000）
     * @return 采样率索引，找不到返回 -1
     */
    int find_sample_rate_index(int nSampleRate);
}
