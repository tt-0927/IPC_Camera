/**
 * @FilePath     : flv_packet_converter.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-13 08:55:30
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-05-13 09:35:52
 * @Description  : FLV写包前码流格式转换工具
 */

#pragma once

#include <cstdint>
#include <vector>

namespace RtmpFlv_NS
{
    /**
     * @brief 将AnnexB格式的视频帧转换为MP4格式（带4字节长度前缀）
     * @param pData AnnexB帧数据
     * @param nSize 帧长度
     * @param vMp4Data 输出MP4格式数据
     * @return true：转换成功，false：数据无效
     */
    bool annexb_to_mp4(const uint8_t* pData, int nSize, std::vector<uint8_t>& vMp4Data);
}
