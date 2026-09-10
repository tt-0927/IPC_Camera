/**
 * @FilePath     : isp_capability_validator.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 11:39:09
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-14 14:25:34
 * @Description  : ISP能力画像内部一致性校验实现
 */

#include "isp_capability_validator.h"

#include <algorithm>

#include "IpcRet.h"

namespace
{
/**
 * @brief   : 校验单个参数范围的合法性
 * @param    {const ISP::IspParamRange_S&} stRange：参数范围
 * @return   {int} OK：合法，ERR_PARAM：非法
 * @note    : 支持的范围需满足 min≤default≤max 且 step>0
 */
int validate_range(const ISP::IspParamRange_S &stRange)
{
    if (!stRange.bSupported)
    {
        return OK;
    }

    if (stRange.nStep <= 0)
    {
        return ERR_PARAM;
    }

    if (stRange.nMin > stRange.nMax)
    {
        return ERR_PARAM;
    }

    if (stRange.nDefault < stRange.nMin || stRange.nDefault > stRange.nMax)
    {
        return ERR_PARAM;
    }

    return OK;
}

/**
 * @brief   : 判断运行场景集合是否包含指定场景
 * @param    {const std::vector<ISP::IspRuntimeScene_E>&} vecScenes：场景集合
 * @param    {ISP::IspRuntimeScene_E} enTarget：目标场景
 * @return   {bool} true：包含
 */
bool has_runtime_scene(const std::vector<ISP::IspRuntimeScene_E> &vecScenes, ISP::IspRuntimeScene_E enTarget)
{
    return std::find(vecScenes.begin(), vecScenes.end(), enTarget) != vecScenes.end();
}
} // namespace

namespace IspCapabilityValidator_NS
{

int validate_profile(const ISP::IspCapabilityProfile_S &stProfile)
{
    /* 规则1：白光-only与红外灯互斥，不能同时为true */
    if (stProfile.bWhiteOnly && stProfile.bSupportRedLight)
    {
        return ERR_PARAM;
    }

    /* 规则2：支持exposure时曝光时间集合不能为空 */
    if (stProfile.stExposure.bSupported && stProfile.stExposure.vecSupportedTimes.empty())
    {
        return ERR_PARAM;
    }

    /* 规则3：支持AWB/NR/mirror/scene时对应enum集合不能为空 */
    if (stProfile.stAwb.bSupported && stProfile.stAwb.vecSupportedModes.empty())
    {
        return ERR_PARAM;
    }

    if (stProfile.stNr.bSupported && stProfile.stNr.vecSupportedModes.empty())
    {
        return ERR_PARAM;
    }

    if (stProfile.stMirror.bSupported && stProfile.stMirror.vecSupportedModes.empty())
    {
        return ERR_PARAM;
    }

    if (stProfile.stScene.bSupported && stProfile.stScene.vecSupportedScenes.empty())
    {
        return ERR_PARAM;
    }

    /* 规则4：支持schedule必须同时支持scene */
    if (stProfile.stScene.bSupportSchedule && !stProfile.stScene.bSupported)
    {
        return ERR_PARAM;
    }

    /* 规则5：所有已支持的range必须满足 min≤default≤max、step>0 */
    int nRet = validate_range(stProfile.stBrightness);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = validate_range(stProfile.stContrast);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = validate_range(stProfile.stSaturation);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = validate_range(stProfile.stSharpness);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = validate_range(stProfile.stGamma);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = validate_range(stProfile.stWhiteLightLevel);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = validate_range(stProfile.stRedLightLevel);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = validate_range(stProfile.stDayNightSensitivity);
    if (nRet != OK)
    {
        return nRet;
    }
    nRet = validate_range(stProfile.stDayNightFilterTime);
    if (nRet != OK)
    {
        return nRet;
    }

    /* 背光子结构range */
    if (stProfile.stBacklight.bSupportWdr)
    {
        nRet = validate_range(stProfile.stBacklight.stWdrLevel);
        if (nRet != OK)
        {
            return nRet;
        }
    }
    if (stProfile.stBacklight.bSupportHlc)
    {
        nRet = validate_range(stProfile.stBacklight.stHlcLevel);
        if (nRet != OK)
        {
            return nRet;
        }
    }

    /* AWB子结构range */
    if (stProfile.stAwb.bSupported)
    {
        nRet = validate_range(stProfile.stAwb.stRGain);
        if (nRet != OK)
        {
            return nRet;
        }
        nRet = validate_range(stProfile.stAwb.stBGain);
        if (nRet != OK)
        {
            return nRet;
        }
    }

    /* NR子结构range */
    if (stProfile.stNr.bSupported)
    {
        nRet = validate_range(stProfile.stNr.stDnrLevel);
        if (nRet != OK)
        {
            return nRet;
        }
        nRet = validate_range(stProfile.stNr.stSnrLevel);
        if (nRet != OK)
        {
            return nRet;
        }
        nRet = validate_range(stProfile.stNr.stTnrLevel);
        if (nRet != OK)
        {
            return nRet;
        }
    }

    /* 规则6：支持红外运行场景(NIGHT_IR/NIGHT_SMART)必须同时支持IR-CUT和红外灯 */
    bool bHasIrScene = has_runtime_scene(stProfile.vecSupportedRuntimeScenes, ISP::IspRuntimeScene_E::NIGHT_IR) ||
                       has_runtime_scene(stProfile.vecSupportedRuntimeScenes, ISP::IspRuntimeScene_E::NIGHT_SMART);

    if (bHasIrScene && (!stProfile.bSupportIrCut || !stProfile.bSupportRedLight))
    {
        return ERR_PARAM;
    }

    return OK;
}

} // namespace IspCapabilityValidator_NS
