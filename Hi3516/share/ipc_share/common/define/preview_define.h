/**
 * @FilePath     : preview_define.h
 * @Author       : 严泽辉 (yanzeh@kfb.cn)
 * @Date         : 2025-07-08 17:18:39
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-09-28 09:50:59
 * @Description  : 预览定义
 */

#pragma once

#include <string>
#include <vector>
#include "common_define.h"

namespace Preview
{
    /// @brief 码流信息
    typedef enum {
        VIDEO_SCALE_ADAPTIVE = 0,   /* 自适应 */
        VIDEO_SCALE_ORIGINAL_SIZE,  /* 原始尺寸 */
        VIDEO_SCALE_ORIGINAL_RATIO, /* 原始比例 */
        VIDEO_SCALE_4_3,            /* 4:3比例 */
        VIDEO_SCALE_16_9            /* 16:9比例 */
    } VIDEO_SCALE_E;

    /// @brief 码流信息
    typedef enum class StreamType
    {
        Main, /* 主码流 */
        Sub   /* 子码流 */
    } StreamType_E;
    typedef struct
    {
        std::string deviceId; /* 设备Id */
        std::string chnId;    /* 通道Id */
    } GB28181NeedInfo_S;

    /**
     * @brief 预览流地址信息
     */
    typedef struct RtspUrl
    {
        std::string strRtspMainUrl;
        std::string strRtspSubUrl;
    } RtspUrl_S;

    /**
     * @brief 预览图像信息
     */
    typedef struct ImageParam
    {
        int nBrightness; /* 亮度[0,100] */
        int nContrast;   /* 对比度[0,100] */
        int nSaturation; /* 饱和度[0,100] */
        int nSharpness;  /* 锐度[0,100] */
    } ImageParam_S;

    /**
     * @brief 预览信息
     */
    typedef struct PreviewInfo
    {
        // int nVolume = 100;
        RtspUrl_S stRtspUrl;
        ImageParam_S stImageParam;
    } PreviewInfo_S;

    /**
     * @brief 采集音频信息
     */
    typedef struct CollectAudioInfo
    {
        int nChn;
        int nCodec;
        int nFormat;
        int nBitRate;
        int nSampleRate;
    } CollectAudioInfo_S;

    /**
     * @brief 对讲信息
     */
    typedef struct IntercomInfo
    {
        bool bEnable;
        std::string strSdp;
        std::string strUrl;
        std::string strLocalIp; /* 对讲当前IP */
    } IntercomInfo_S;

    /**
     * @brief 广播信息
     */
    typedef struct BroadcastInfo
    {
        bool bEnable;
        std::string strSdp;
        std::string strUrl;
        std::string strLocalIp; /* 广播当前IP */
    } BroadcastInfo_S;

    /**
     * @brief 蜂鸣器报警
     */
    typedef struct BeepAlarm
    {
        bool bEnable;
    } BeepAlarm_S;

} // namespace Preview
