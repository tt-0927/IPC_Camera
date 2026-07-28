/**
 * @file CapabilityInfoConvert.h
 * @author tianl (tianl@kfb.cn)
 * @date 2026-07-28
 * @LastEditors  : qinjt@kfb.cn
 * @LastEditTime : 2026-07-28
 *
 * @brief CapabilityInfoConvert 模块接口与类型定义
 * 功能说明：
 * 1. 声明 CapabilityInfoConvert 模块对外接口和数据类型
 * 2. 定义模块依赖的常量、回调或辅助类型
 * 3. 为调用方提供明确且稳定的编译期契约
 */
#ifndef NETSDK_CAPABILITY_INFO_CONVERT_H
#define NETSDK_CAPABILITY_INFO_CONVERT_H

#include <string>
#include <vector>
#include <set>

#include "Json.h"

/* 库通用头文件 */
#ifdef NET_TV_SDK_SERVER_API
    #include "NetTVSDKServerInterface.h"
#elif defined(NET_TV_SDK_CLIENT_API)
    #include "NetTVSDKClientInterface.h"
#else
    #include "NetTVSDKCommon.h"
#endif

namespace SDKConvert
{
    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 视频分辨率结构体转换
     */
    void deal(Json::Object* pRootJson, NET_VideoResolution_S& stInfo, bool bOutStruct);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 取值范围结构体转换
     */
    void deal(Json::Object* pRootJson, NET_Range_S& stInfo, bool bOutStruct);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 视频编码参数配置转换
     */
    void deal(Json::Object* pRootJson, NET_VideoEncodeOption_S& stInfo, bool bOutStruct);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 视频编码格式能力转换
     */
    void deal(Json::Object* pRootJson, NET_VideoEncodeAbility_S& stInfo, bool bOutStruct);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 视频码流参数能力集转换 (单个码流)
     */
    void deal(Json::Object* pRootJson, NET_VideoStreamCap_S& stInfo, bool bOutStruct);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 视频编码能力集转换 (多码流, NET_TV_CAP_VIDEO_ENCODE)
     */
    void deal(Json::Object* pRootJson, NET_VideoEncodeCap_S& stInfo, bool bOutStruct);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 音频范围转换
     */
    void deal(Json::Object* pRootJson, NET_AudioRange_S& stInfo, bool bOutStruct);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 音频格式能力转换
     */
    void deal(Json::Object* pRootJson, NET_AudioFormatCap_S& stInfo, bool bOutStruct);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief 音频编码能力集转换 (多码流, NET_TV_CAP_VIDEO_ENCODE)
     */
    void deal(Json::Object* pRootJson, NET_AudioCap_S& stInfo, bool bOutStruct);

    /**
 * @author tianl (tianl@kfb.cn)
     * @brief OSD参数能力集转换 (NET_TV_CAP_OSD)
     */
    void deal(Json::Object* pRootJson, NET_OsdCap_S& stInfo, bool bOutStruct);

    /* ==================== 后续扩展能力集 ==================== */
    /* NET_TV_CAP_OSD: void deal(Json::Object*, NET_TV_OSD_CAP_S&, bool); */
    /* NET_TV_CAP_SMART: void deal(Json::Object*, NET_TV_SMART_CAP_S&, bool); */
    /* NET_TV_CAP_IMAGE: void deal(Json::Object*, NET_TV_IMAGE_CAP_S&, bool); */
    /* NET_TV_CAP_AUDIO: void deal(Json::Object*, NET_TV_AUDIO_CAP_S&, bool); */
};

#endif
