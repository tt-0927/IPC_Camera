/**
 * @FilePath     : isp_param_policy.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-10 15:19:22
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-23 09:17:09
 * @Description  : 日夜与补光裁决纯策略工具声明
 */

#include "isp_param_policy.h"

#include <algorithm>

#include "IpcRet.h"

namespace
{
/**
 * @brief   : 确认能力范围是否支持
 * @param    {ISP::IspParamRange_S} stRange：能力范围
 * @param    {const char*} pName：参数名称
 * @return   {int} OK：支持，ERR_UNSUPPORT：不支持
 */
int ensure_range_supported(const ISP::IspParamRange_S &stRange, const char *pName)
{
    (void)pName;

    if (!stRange.bSupported)
    {
        return ERR_UNSUPPORT;
    }

    return OK;
}

/**
 * @brief   : 按能力范围裁剪数值
 * @param    {long long} nValue：原始数值
 * @param    {ISP::IspParamRange_S} stRange：能力范围
 * @param    {const char*} pName：参数名称
 * @return   {int} 裁剪后的数值
 */
int clamp_to_range(long long nValue, const ISP::IspParamRange_S &stRange, const char *pName)
{
    (void)pName;

    if (nValue < stRange.nMin)
    {
        return stRange.nMin;
    }

    if (nValue > stRange.nMax)
    {
        return stRange.nMax;
    }

    return static_cast<int>(nValue);
}

/**
 * @brief   : 判断补光类型是否需要红外能力
 * @param    {ISP::LightType_E} enLightType：补光类型
 * @return   {bool} true：需要红外能力，false：不需要
 * @note    : SMART类型不强制要求红外能力，白光-only设备也可使用智能补光（回退为白光）
 */
bool is_red_light_required(ISP::LightType_E enLightType)
{
    return enLightType == ISP::LIGHT_TYPE_RED ||
           enLightType == ISP::LIGHT_TYPE_RED_ON_WHITE_OFF;
}

/**
 * @brief   : 判断补光类型是否需要白光亮度范围
 * @param    {ISP::LightType_E} enLightType：补光类型
 * @return   {bool} true：需要白光亮度范围，false：不需要
 */
bool is_white_light_type(ISP::LightType_E enLightType)
{
    return enLightType == ISP::LIGHT_TYPE_WHITE || enLightType == ISP::LIGHT_TYPE_WHITE_ON_RED_OFF;
}

/**
 * @brief   : 校验日夜定时时刻字段
 * @param    {const Common::Time_S&} stTime：待校验时刻
 * @return   {bool} true：时刻合法，false：时分秒或毫秒越界
 */
bool is_valid_time_of_day(const Common::Time_S &stTime)
{
    return stTime.nHour < 24U && stTime.nMinute < 60U && stTime.nSecond < 60U && stTime.nMilliSec < 1000U;
}

/**
 * @brief   : 判断日夜定时区间是否严格递增
 * @param    {const ISP::DayNightAttr_S&} stConfig：已校验时刻字段的日夜配置
 * @return   {bool} true：开始时间早于结束时间，false：开始时间不早于结束时间
 * @note    : 定时日夜模式不支持跨零点，避免一个配置同时表达当天和次日的运行状态。
 */
bool is_daynight_time_range_increasing(const ISP::DayNightAttr_S &stConfig)
{
    /* 从当天00:00开始计算的开始时间毫秒数。 */
    const unsigned long long u64BeginTime = (((static_cast<unsigned long long>(stConfig.stBeginTime.nHour) * 60U +
                                               stConfig.stBeginTime.nMinute) *
                                                  60U +
                                              stConfig.stBeginTime.nSecond) *
                                             1000U) +
                                            stConfig.stBeginTime.nMilliSec;
    /* 从当天00:00开始计算的结束时间毫秒数。 */
    const unsigned long long u64EndTime = (((static_cast<unsigned long long>(stConfig.stEndTime.nHour) * 60U +
                                             stConfig.stEndTime.nMinute) *
                                                60U +
                                            stConfig.stEndTime.nSecond) *
                                           1000U) +
                                          stConfig.stEndTime.nMilliSec;

    return u64BeginTime < u64EndTime;
}

/**
 * @brief   : 判断枚举值是否在支持集合中
 * @param    {const T&} enValue：待检查的枚举值
 * @param    {const std::vector<T>&} vecSupported：支持的枚举集合
 * @return   {bool} true：在集合中
 */
template <typename T>
bool is_enum_supported(const T &enValue, const std::vector<T> &vecSupported)
{
    return std::find(vecSupported.begin(), vecSupported.end(), enValue) != vecSupported.end();
}
} // namespace

namespace IspParamPolicy_NS
{

int normalize_image_param(ISP::ImageParam_S &stConfig, const ISP::IspCapabilityProfile_S &stProfile)
{
    /* nRet 依次承接各能力范围校验，任一基础参数不支持即拒绝整组配置。 */
    int nRet = ensure_range_supported(stProfile.stBrightness, "brightness");
    if (nRet != OK)
    {
        return nRet;
    }

    nRet = ensure_range_supported(stProfile.stContrast, "contrast");
    if (nRet != OK)
    {
        return nRet;
    }

    nRet = ensure_range_supported(stProfile.stSaturation, "saturation");
    if (nRet != OK)
    {
        return nRet;
    }

    nRet = ensure_range_supported(stProfile.stSharpness, "sharpness");
    if (nRet != OK)
    {
        return nRet;
    }

    /* step: 全部范围已确认后统一裁剪，输出配置可被平台层直接下发。 */
    stConfig.nBrightness = static_cast<unsigned int>(clamp_to_range(stConfig.nBrightness, stProfile.stBrightness, "brightness"));
    stConfig.nContrast = static_cast<unsigned int>(clamp_to_range(stConfig.nContrast, stProfile.stContrast, "contrast"));
    stConfig.nSaturation = static_cast<unsigned int>(clamp_to_range(stConfig.nSaturation, stProfile.stSaturation, "saturation"));
    stConfig.nSharpness = static_cast<unsigned int>(clamp_to_range(stConfig.nSharpness, stProfile.stSharpness, "sharpness"));
    return OK;
}

int normalize_daynight(ISP::DayNightAttr_S &stConfig, const ISP::IspCapabilityProfile_S &stProfile)
{
    /* ! 定时模式只能表达同一天内的非空区间，开始时间等于或晚于结束时间时直接拒绝。 */
    if (stConfig.enDayNightMode == ISP::TIME_MODE &&
        (!is_valid_time_of_day(stConfig.stBeginTime) || !is_valid_time_of_day(stConfig.stEndTime) ||
         !is_daynight_time_range_increasing(stConfig)))
    {
        return ERR_PARAM;
    }

    /* 双灯同时开启无法映射为单一运行场景，必须在共享策略层拒绝。 */
    if (stConfig.stFillLight.enLightType == ISP::LIGHT_TYPE_BOTH)
    {
        return ERR_PARAM;
    }

    if (is_red_light_required(stConfig.stFillLight.enLightType) && (!stProfile.bSupportIrCut || !stProfile.bSupportRedLight))
    {
        return ERR_UNSUPPORT;
    }

    /* 先确认日夜公共参数范围，再按实际补光类型校验对应灯光亮度范围。 */
    int nRet = ensure_range_supported(stProfile.stDayNightSensitivity, "dayNightSensitivity");
    if (nRet != OK)
    {
        return nRet;
    }

    nRet = ensure_range_supported(stProfile.stDayNightFilterTime, "dayNightFilterTime");
    if (nRet != OK)
    {
        return nRet;
    }

    if (is_white_light_type(stConfig.stFillLight.enLightType))
    {
        nRet = ensure_range_supported(stProfile.stWhiteLightLevel, "whiteLightLevel");
        if (nRet != OK)
        {
            return nRet;
        }
    }

    if (is_red_light_required(stConfig.stFillLight.enLightType))
    {
        nRet = ensure_range_supported(stProfile.stRedLightLevel, "redLightLevel");
        if (nRet != OK)
        {
            return nRet;
        }
    }

    /* 仅修改能力约束相关字段，时间和模式等其他配置保持调用方原值。 */
    stConfig.nSensitivityLevel = static_cast<unsigned int>(
        clamp_to_range(stConfig.nSensitivityLevel, stProfile.stDayNightSensitivity, "dayNightSensitivity"));
    stConfig.nFilterTime = static_cast<unsigned int>(
        clamp_to_range(stConfig.nFilterTime, stProfile.stDayNightFilterTime, "dayNightFilterTime"));
    if (is_white_light_type(stConfig.stFillLight.enLightType) || stProfile.stWhiteLightLevel.bSupported)
    {
        stConfig.stFillLight.stWhiteAttr.nLightLevel = clamp_to_range(stConfig.stFillLight.stWhiteAttr.nLightLevel,
                                                                      stProfile.stWhiteLightLevel,
                                                                      "whiteLightLevel");
    }
    if (is_red_light_required(stConfig.stFillLight.enLightType) || stProfile.stRedLightLevel.bSupported)
    {
        stConfig.stFillLight.stRedAttr.nLightLevel = clamp_to_range(stConfig.stFillLight.stRedAttr.nLightLevel,
                                                                    stProfile.stRedLightLevel,
                                                                    "redLightLevel");
    }

    return OK;
}

int normalize_exposure(ISP::ExposureAttr_S &stConfig, const ISP::IspCapabilityProfile_S &stProfile)
{
    if (!stProfile.stExposure.bSupported)
    {
        return ERR_UNSUPPORT;
    }

    if (!is_enum_supported(stConfig.enExpTime, stProfile.stExposure.vecSupportedTimes))
    {
        return ERR_PARAM;
    }

    return OK;
}

int normalize_backlight(ISP::BackLightArrt_S &stConfig, const ISP::IspCapabilityProfile_S &stProfile)
{
    if (!stProfile.stBacklight.bSupported)
    {
        return ERR_UNSUPPORT;
    }

    /* WDR/HLC在不支持时被启用视为非法配置。 */
    if (stConfig.stWdrAttr.bEnable && !stProfile.stBacklight.bSupportWdr)
    {
        return ERR_PARAM;
    }

    if (stConfig.stHlsAttr.bEnable && !stProfile.stBacklight.bSupportHlc)
    {
        return ERR_PARAM;
    }

    /* 按能力范围裁剪WDR/HLC等级。 */
    if (stConfig.stWdrAttr.bEnable && stProfile.stBacklight.bSupportWdr)
    {
        stConfig.stWdrAttr.nWdrLevel = clamp_to_range(stConfig.stWdrAttr.nWdrLevel,
                                                      stProfile.stBacklight.stWdrLevel,
                                                      "wdrLevel");
    }

    if (stConfig.stHlsAttr.bEnable && stProfile.stBacklight.bSupportHlc)
    {
        stConfig.stHlsAttr.nHlsLevel = clamp_to_range(stConfig.stHlsAttr.nHlsLevel,
                                                      stProfile.stBacklight.stHlcLevel,
                                                      "hlcLevel");
    }

    return OK;
}

int normalize_awb(ISP::AwbAttr_S &stConfig, const ISP::IspCapabilityProfile_S &stProfile)
{
    if (!stProfile.stAwb.bSupported)
    {
        return ERR_UNSUPPORT;
    }

    if (!is_enum_supported(stConfig.enAwbMode, stProfile.stAwb.vecSupportedModes))
    {
        return ERR_PARAM;
    }

    stConfig.nRGain = static_cast<unsigned int>(
        clamp_to_range(stConfig.nRGain, stProfile.stAwb.stRGain, "rGain"));
    stConfig.nBGain = static_cast<unsigned int>(
        clamp_to_range(stConfig.nBGain, stProfile.stAwb.stBGain, "bGain"));

    return OK;
}

int normalize_nr(ISP::DnrAttr_S &stConfig, const ISP::IspCapabilityProfile_S &stProfile)
{
    if (!stProfile.stNr.bSupported)
    {
        return ERR_UNSUPPORT;
    }

    if (!is_enum_supported(stConfig.enDnrMode, stProfile.stNr.vecSupportedModes))
    {
        return ERR_PARAM;
    }

    stConfig.nDnrLevel = static_cast<unsigned int>(
        clamp_to_range(stConfig.nDnrLevel, stProfile.stNr.stDnrLevel, "dnrLevel"));
    stConfig.nSnrLevel = static_cast<unsigned int>(
        clamp_to_range(stConfig.nSnrLevel, stProfile.stNr.stSnrLevel, "snrLevel"));
    stConfig.nTnrLevel = static_cast<unsigned int>(
        clamp_to_range(stConfig.nTnrLevel, stProfile.stNr.stTnrLevel, "tnrLevel"));

    return OK;
}

int normalize_mirror(ISP::VideoAdjust_S &stConfig, const ISP::IspCapabilityProfile_S &stProfile)
{
    if (!stProfile.stMirror.bSupported)
    {
        return ERR_UNSUPPORT;
    }

    if (!is_enum_supported(stConfig.enMirrorMode, stProfile.stMirror.vecSupportedModes))
    {
        return ERR_PARAM;
    }

    return OK;
}

int normalize_scene(ISP::SceneType_E &enScene, const ISP::IspCapabilityProfile_S &stProfile)
{
    if (!stProfile.stScene.bSupported)
    {
        return ERR_UNSUPPORT;
    }

    if (!is_enum_supported(enScene, stProfile.stScene.vecSupportedScenes))
    {
        return ERR_PARAM;
    }

    return OK;
}

} // namespace IspParamPolicy_NS
