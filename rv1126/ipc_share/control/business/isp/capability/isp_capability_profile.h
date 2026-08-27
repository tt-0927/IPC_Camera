/**
 * @FilePath     : isp_capability_profile.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-10 15:03:18
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-27 19:26:20
 * @Description  : ISP平台功能和参数范围模型定义
 */

#pragma once

#include <vector>

#include "isp_define.h"
#include "isp_runtime_scene.h"

namespace ISP
{

/**
 * @brief 参数范围和默认值描述。
 */
struct IspParamRange_S
{
    /* 参数是否被当前平台或机型支持 */
    bool bSupported;
    /* 参数允许的最小值 */
    int nMin;
    /* 参数允许的最大值 */
    int nMax;
    /* 参数调节步进 */
    int nStep;
    /* 参数默认值 */
    int nDefault;

    /**
     * @brief   : 构造默认参数范围
     * @return  : 无
     * @note    : 默认标记为不支持，调用方需以bSupported判断范围是否有效
     */
    IspParamRange_S() : bSupported(false), nMin(0), nMax(0), nStep(1), nDefault(0)
    {
    }
};

/**
 * @brief 曝光能力描述。
 */
struct IspExposureCapability_S
{
    /* 是否支持手动曝光配置 */
    bool bSupported;
    /* 是否支持防横纹 */
    bool bSupportAntiBanding;
    /* 支持的曝光时间档位集合 */
    std::vector<ExpTimeMode_E> vecSupportedTimes;

    IspExposureCapability_S() : bSupported(false), bSupportAntiBanding(false)
    {
    }
};

/**
 * @brief 背光补偿能力描述。
 */
struct IspBacklightCapability_S
{
    /* 是否支持背光补偿区域配置 */
    bool bSupported;
    /* 是否支持背光补偿(BLC) */
    bool bSupportBlc;
    /* 是否支持宽动态(WDR) */
    bool bSupportWdr;
    /* 是否支持强光抑制(HLC) */
    bool bSupportHlc;
    /* 宽动态等级范围 */
    IspParamRange_S stWdrLevel;
    /* 强光抑制等级范围 */
    IspParamRange_S stHlcLevel;

    IspBacklightCapability_S() : bSupported(false), bSupportBlc(false), bSupportWdr(false), bSupportHlc(false)
    {
    }
};

/**
 * @brief 白平衡能力描述。
 */
struct IspAwbCapability_S
{
    /* 是否支持白平衡配置 */
    bool bSupported;
    /* 支持的白平衡模式集合 */
    std::vector<AwbMode_E> vecSupportedModes;
    /* R增益范围 */
    IspParamRange_S stRGain;
    /* B增益范围 */
    IspParamRange_S stBGain;

    IspAwbCapability_S() : bSupported(false)
    {
    }
};

/**
 * @brief 降噪能力描述。
 */
struct IspNrCapability_S
{
    /* 是否支持降噪配置 */
    bool bSupported;
    /* 支持的降噪模式集合 */
    std::vector<DnrMode_E> vecSupportedModes;
    /* 降噪等级范围(普通模式) */
    IspParamRange_S stDnrLevel;
    /* 空域降噪等级范围(高级模式) */
    IspParamRange_S stSnrLevel;
    /* 时域降噪等级范围(高级模式) */
    IspParamRange_S stTnrLevel;

    IspNrCapability_S() : bSupported(false)
    {
    }
};

/**
 * @brief 镜像翻转能力描述。
 */
struct IspMirrorCapability_S
{
    /* 是否支持镜像翻转配置 */
    bool bSupported;
    /* 支持的镜像模式集合 */
    std::vector<MirrorMode_E> vecSupportedModes;

    IspMirrorCapability_S() : bSupported(false)
    {
    }
};

/**
 * @brief 场景能力描述。
 */
struct IspSceneCapability_S
{
    /* 是否支持场景配置 */
    bool bSupported;
    /* 是否支持场景计划 */
    bool bSupportSchedule;
    /* 支持的网页配置场景类型集合 */
    std::vector<SceneType_E> vecSupportedScenes;

    IspSceneCapability_S() : bSupported(false), bSupportSchedule(false)
    {
    }
};

/**
 * @brief 当前平台和机型支持的 ISP 功能和参数范围。
 */
struct IspCapabilityProfile_S
{
    /* 当前机型是否支持IR-CUT硬件切换 */
    bool bSupportIrCut;
    /* 当前机型是否仅支持白光补光 */
    bool bWhiteOnly;
    /* 当前机型是否支持红外补光 */
    bool bSupportRedLight;
    /* 亮度参数范围 */
    IspParamRange_S stBrightness;
    /* 对比度参数范围 */
    IspParamRange_S stContrast;
    /* 饱和度参数范围 */
    IspParamRange_S stSaturation;
    /* 锐度参数范围 */
    IspParamRange_S stSharpness;
    /* Gamma参数范围 */
    IspParamRange_S stGamma;
    /* 白光灯亮度等级范围 */
    IspParamRange_S stWhiteLightLevel;
    /* 红外灯亮度等级范围 */
    IspParamRange_S stRedLightLevel;
    /* 日夜切换灵敏度范围 */
    IspParamRange_S stDayNightSensitivity;
    /* 日夜切换过滤时间范围 */
    IspParamRange_S stDayNightFilterTime;

    /* 曝光能力 */
    IspExposureCapability_S stExposure;
    /* 背光能力 */
    IspBacklightCapability_S stBacklight;
    /* 白平衡能力 */
    IspAwbCapability_S stAwb;
    /* 降噪能力 */
    IspNrCapability_S stNr;
    /* 镜像能力 */
    IspMirrorCapability_S stMirror;
    /* 场景能力 */
    IspSceneCapability_S stScene;

    /* 当前平台支持的运行时ISP场景集合 */
    std::vector<IspRuntimeScene_E> vecSupportedRuntimeScenes;

    /**
     * @brief   : 构造默认 ISP 功能和参数范围
     * @return  : 无
     * @note    : 默认关闭所有能力开关，具体能力由平台实现填充
     */
    IspCapabilityProfile_S() : bSupportIrCut(false), bWhiteOnly(false), bSupportRedLight(false)
    {
    }
};

} // namespace ISP
