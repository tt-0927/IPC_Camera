/**
 * @FilePath     : isp_daynight_policy.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-10 15:19:22
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-15 15:16:08
 * @Description  : 日夜与补光裁决纯策略工具实现
 */

#include "isp_daynight_policy.h"

#include <algorithm>

#include "IpcRet.h"

namespace
{
/* 一天的总秒数，用于跨零点区间归一化。 */
constexpr int SECONDS_PER_DAY = 24 * 60 * 60;

/**
 * @brief   : 比较两组单灯属性是否完全一致
 * @param    {ISP::Light_S} stLeft：待比较的左侧灯光属性
 * @param    {ISP::Light_S} stRight：待比较的右侧灯光属性
 * @return   {bool} true：一致，false：不一致
 */
bool is_light_attr_same(const ISP::Light_S &stLeft, const ISP::Light_S &stRight)
{
    return stLeft.bEnable == stRight.bEnable && stLeft.nLightLevel == stRight.nLightLevel;
}

/**
 * @brief   : 比较两组补光类型是否一致
 * @param    {ISP::FillLight_S} stLeft：待比较的左侧补光配置
 * @param    {ISP::FillLight_S} stRight：待比较的右侧补光配置
 * @return   {bool} true：一致，false：不一致
 */
bool is_fill_light_type_same(const ISP::FillLight_S &stLeft, const ISP::FillLight_S &stRight)
{
    return stLeft.enLightType == stRight.enLightType;
}

/**
 * @brief   : 比较两组补光亮度参数是否一致
 * @param    {ISP::FillLight_S} stLeft：待比较的左侧补光配置
 * @param    {ISP::FillLight_S} stRight：待比较的右侧补光配置
 * @return   {bool} true：一致，false：不一致
 * @note    : 这里只比较白光和红外的亮度相关参数，不比较补光类型
 */
bool is_fill_light_level_same(const ISP::FillLight_S &stLeft, const ISP::FillLight_S &stRight)
{
    return is_light_attr_same(stLeft.stWhiteAttr, stRight.stWhiteAttr) &&
           is_light_attr_same(stLeft.stRedAttr, stRight.stRedAttr);
}

/**
 * @brief   : 将日夜时间归一化为当日秒数
 * @param    {Common::Time_S} stTime：日夜时间
 * @return   {int} 当日秒数
 */
int to_sec_of_day(const Common::Time_S &stTime)
{
    return stTime.nHour * 3600 + stTime.nMinute * 60 + stTime.nSecond;
}
} // namespace

namespace IspDayNightPolicy_NS
{

ISP::IspSmartLightDecision_E decide_smart_light(const ISP::IspCapabilityProfile_S &stProfile)
{
    (void)stProfile;
    /* note: 当前智能补光默认按红外执行，后续扩展为基于画面值的动态策略。 */
    return ISP::IspSmartLightDecision_E::USE_RED;
}

int ensure_scene_supported(const ISP::IspCapabilityProfile_S &stProfile, ISP::IspRuntimeScene_E enScene)
{
    /* 空集合表示调用方未声明额外限制，兼容旧机型能力画像。 */
    if (stProfile.vecSupportedRuntimeScenes.empty())
    {
        return OK;
    }

    /* 在平台声明的合法运行场景中查找策略输出，禁止把不支持的目标交给业务侧。 */
    const auto it = std::find(stProfile.vecSupportedRuntimeScenes.begin(), stProfile.vecSupportedRuntimeScenes.end(), enScene);
    if (it == stProfile.vecSupportedRuntimeScenes.end())
    {
        return ERR_UNSUPPORT;
    }

    return OK;
}

int decide_runtime_scene(bool bIsNight,
                         const ISP::DayNightAttr_S &stConfig,
                         const ISP::IspCapabilityProfile_S &stProfile,
                         ISP::IspRuntimeDecision_S &stDecision)
{
    /* 每次裁决先清空输出，避免调用方复用对象时残留上轮 IR-CUT 或补光目标。 */
    stDecision = ISP::IspRuntimeDecision_S();
    stDecision.bIsNight = bIsNight;
    stDecision.bNeedFillLightSync = true;

    if (!bIsNight)
    {
        /* 白天关闭补光并按能力决定是否将 IR-CUT 复位到白天位置。 */
        stDecision.enRuntimeScene = ISP::IspRuntimeScene_E::DAY;
        stDecision.enEffectiveLightType = ISP::LIGHT_TYPE_CLOSE;
        stDecision.enIrCutTarget = stProfile.bSupportIrCut ? ISP::IspIrCutTarget_E::DAY : ISP::IspIrCutTarget_E::NONE;
        stDecision.bNeedIrCutSwitch = stProfile.bSupportIrCut;
        return ensure_scene_supported(stProfile, stDecision.enRuntimeScene);
    }

    /* 初始有效补光类型来自配置；智能补光分支可能按纯策略修正该值。 */
    stDecision.enEffectiveLightType = stConfig.stFillLight.enLightType;

    switch (stConfig.stFillLight.enLightType)
    {
    case ISP::LIGHT_TYPE_WHITE:
    case ISP::LIGHT_TYPE_WHITE_ON_RED_OFF:
        stDecision.enRuntimeScene = ISP::IspRuntimeScene_E::NIGHT_WHITE;
        stDecision.enEffectiveLightType = ISP::LIGHT_TYPE_WHITE;
        stDecision.enIrCutTarget = stProfile.bSupportIrCut ? ISP::IspIrCutTarget_E::DAY : ISP::IspIrCutTarget_E::NONE;
        stDecision.bNeedIrCutSwitch = stProfile.bSupportIrCut;
        break;

    case ISP::LIGHT_TYPE_RED:
    case ISP::LIGHT_TYPE_RED_ON_WHITE_OFF:
        if (!stProfile.bSupportIrCut || !stProfile.bSupportRedLight)
        {
            return ERR_UNSUPPORT;
        }
        stDecision.enRuntimeScene = ISP::IspRuntimeScene_E::NIGHT_IR;
        stDecision.enEffectiveLightType = ISP::LIGHT_TYPE_RED;
        stDecision.enIrCutTarget = ISP::IspIrCutTarget_E::NIGHT;
        stDecision.bNeedIrCutSwitch = true;
        break;

    case ISP::LIGHT_TYPE_SMART:
        if (!stProfile.bSupportIrCut || !stProfile.bSupportRedLight)
        {
            return ERR_UNSUPPORT;
        }
        {
            /* 智能补光的选择保持为纯计算结果，实际硬件操作由业务侧编排器执行。 */
            ISP::IspSmartLightDecision_E enSmartDecision = decide_smart_light(stProfile);
            if (enSmartDecision == ISP::IspSmartLightDecision_E::USE_RED)
            {
                stDecision.enRuntimeScene = ISP::IspRuntimeScene_E::NIGHT_SMART;
                /* info: SMART是用户策略类型；裁决后必须向外设层输出可直接操作的红外硬件灯型。 */
                stDecision.enEffectiveLightType = ISP::LIGHT_TYPE_RED;
                stDecision.enIrCutTarget = ISP::IspIrCutTarget_E::NIGHT;
                stDecision.bNeedIrCutSwitch = true;
            }
            /* note: USE_WHITE和KEEP_CURRENT分支后续扩展，当前固定按红外执行。 */
        }
        break;

    case ISP::LIGHT_TYPE_CLOSE:
        stDecision.enRuntimeScene = ISP::IspRuntimeScene_E::NIGHT_LIGHT_OFF;
        stDecision.enEffectiveLightType = ISP::LIGHT_TYPE_CLOSE;
        stDecision.enIrCutTarget = stProfile.bSupportIrCut ? ISP::IspIrCutTarget_E::NIGHT : ISP::IspIrCutTarget_E::NONE;
        stDecision.bNeedIrCutSwitch = stProfile.bSupportIrCut;
        break;

    default:
        return ERR_PARAM;
    }

    return ensure_scene_supported(stProfile, stDecision.enRuntimeScene);
}

bool is_night_by_time_range(const ISP::DayNightAttr_S &stConfig, int nNowSecOfDay)
{
    /* review: 夜晚区间[begin,end)，begin>end时视为跨零点(如19:00-07:00)。 */
    /* 起止时间与当前时间统一为当天秒数，后续分支只处理区间关系。 */
    int nBegin = to_sec_of_day(stConfig.stBeginTime);
    int nEnd = to_sec_of_day(stConfig.stEndTime);
    int nNow = ((nNowSecOfDay % SECONDS_PER_DAY) + SECONDS_PER_DAY) % SECONDS_PER_DAY;

    if (nBegin == nEnd)
    {
        return false;
    }

    if (nBegin < nEnd)
    {
        return nNow >= nBegin && nNow < nEnd;
    }

    return nNow >= nBegin || nNow < nEnd;
}

bool has_night_light_runtime_changed(const ISP::DayNightAttr_S &stOld, const ISP::DayNightAttr_S &stNew)
{
    /* 防补光过曝仅在夜间补光运行态中有意义，变化后需要立即重同步夜间策略。 */
    if (stOld.bFillLightExp != stNew.bFillLightExp)
    {
        return true;
    }

    /* 光控模式变化会改变灯光执行策略，即使补光类型和亮度数值未变化也需重同步。 */
    if (stOld.enLightMode != stNew.enLightMode)
    {
        return true;
    }

    if (!is_fill_light_type_same(stOld.stFillLight, stNew.stFillLight))
    {
        return true;
    }

    if (stNew.enLightMode == ISP::LightBrightMode_E::MANUAL_LIGHT_BRIGHT &&
        !is_fill_light_level_same(stOld.stFillLight, stNew.stFillLight))
    {
        return true;
    }

    return false;
}

} // namespace IspDayNightPolicy_NS
