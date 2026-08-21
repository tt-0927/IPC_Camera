/**
 * @FilePath     : isp_capability_builder.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-10 15:17:16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-27 19:26:20
 * @Description  : Hi3516 ISP功能和参数范围构建实现
 */

#include "isp_capability_builder.h"

#include "IpcRet.h"
#include "isp_define.h"
#include "isp_capability_validator.h"

namespace
{
/* ISP网页参数统一使用的最小值。 */
constexpr int ISP_USER_PARAM_MIN = 0;
/* ISP网页参数统一使用的最大值。 */
constexpr int ISP_USER_PARAM_MAX = 100;
/* ISP网页参数统一使用的步进值。 */
constexpr int ISP_USER_PARAM_STEP = 1;
/* ISP网页参数统一使用的默认值。 */
constexpr int ISP_USER_PARAM_DEFAULT = 50;
/* 自动日夜灵敏度最小等级。 */
constexpr int DAY_NIGHT_SENSITIVITY_MIN = 1;
/* 自动日夜灵敏度最大等级。 */
constexpr int DAY_NIGHT_SENSITIVITY_MAX = 7;
/* 自动日夜灵敏度默认等级。 */
constexpr int DAY_NIGHT_SENSITIVITY_DEFAULT = 2;
/* 自动日夜过滤时间默认值，单位为秒。 */
constexpr int DAY_NIGHT_FILTER_TIME_DEFAULT = 15;

/**
 * @brief   : 填充一个已支持的整数范围
 * @param    {ISP::IspParamRange_S} stRange：待填充的范围
 * @param    {int} nMin：最小值
 * @param    {int} nMax：最大值
 * @param    {int} nStep：步进
 * @param    {int} nDefault：默认值
 * @return   {void}
 */
void fill_supported_range(ISP::IspParamRange_S &stRange, int nMin, int nMax, int nStep, int nDefault)
{
    stRange.bSupported = true;
    stRange.nMin = nMin;
    stRange.nMax = nMax;
    stRange.nStep = nStep;
    stRange.nDefault = nDefault;
}

/**
 * @brief   : 填充一个网页百分比参数范围
 * @param    {ISP::IspParamRange_S} stRange：待填充的范围
 * @return   {void}
 */
void fill_user_percent_range(ISP::IspParamRange_S &stRange)
{
    fill_supported_range(stRange, ISP_USER_PARAM_MIN, ISP_USER_PARAM_MAX, ISP_USER_PARAM_STEP, ISP_USER_PARAM_DEFAULT);
}
} // namespace

namespace CapabilityBuilder_NS
{

int build_profile(ISP::IspCapabilityProfile_S &stProfile)
{
    /* 清空输出，避免机型宏变更后沿用上次构建的功能标记。 */
    stProfile = ISP::IspCapabilityProfile_S();

#if CAP_ISP_IR_SWITCH
    stProfile.bSupportIrCut = true;
#else
    stProfile.bSupportIrCut = false;
#endif

#if CAP_LIGHT_WHITE_ONLY
    stProfile.bWhiteOnly = true;
#else
    stProfile.bWhiteOnly = false;
#endif

    stProfile.bSupportRedLight = !stProfile.bWhiteOnly;

    /* step: 填写网页可配置项的范围；共享层据此校验并修正参数。 */
    fill_user_percent_range(stProfile.stBrightness);
    fill_user_percent_range(stProfile.stContrast);
    fill_user_percent_range(stProfile.stSaturation);
    fill_user_percent_range(stProfile.stSharpness);
    fill_user_percent_range(stProfile.stWhiteLightLevel);
    fill_user_percent_range(stProfile.stRedLightLevel);
    fill_supported_range(stProfile.stDayNightSensitivity,
                         DAY_NIGHT_SENSITIVITY_MIN,
                         DAY_NIGHT_SENSITIVITY_MAX,
                         ISP_USER_PARAM_STEP,
                         DAY_NIGHT_SENSITIVITY_DEFAULT);
    fill_supported_range(stProfile.stDayNightFilterTime,
                         FILTER_TIME_MIN,
                         FILTER_TIME_MAX,
                         ISP_USER_PARAM_STEP,
                         DAY_NIGHT_FILTER_TIME_DEFAULT);

    /* 日夜选择只能输出此集合中的场景，不能把平台不支持的场景交给硬件。 */
    stProfile.vecSupportedRuntimeScenes = {
        ISP::IspRuntimeScene_E::DAY,
        ISP::IspRuntimeScene_E::NIGHT_WHITE,
        ISP::IspRuntimeScene_E::NIGHT_LIGHT_OFF,
    };
    if (stProfile.bSupportIrCut && stProfile.bSupportRedLight)
    {
        stProfile.vecSupportedRuntimeScenes.push_back(ISP::IspRuntimeScene_E::NIGHT_IR);
        stProfile.vecSupportedRuntimeScenes.push_back(ISP::IspRuntimeScene_E::NIGHT_SMART);
    }

    /* step: 填写固定命令的全部能力字段，供共享层校验参数。 */
    stProfile.stExposure.bSupported = true;
    stProfile.stExposure.bSupportAntiBanding = true;
    stProfile.stExposure.vecSupportedTimes = { ISP::One_3,    ISP::One_6,    ISP::One_12,    ISP::One_25,
                                               ISP::One_50,   ISP::One_100,  ISP::One_150,   ISP::One_200,
                                               ISP::One_250,  ISP::One_500,  ISP::One_750,   ISP::One_1000,
                                               ISP::One_2000, ISP::One_4000, ISP::One_10000, ISP::One_100000 };

    stProfile.stBacklight.bSupported = true;
    stProfile.stBacklight.bSupportBlc = true;
    stProfile.stBacklight.bSupportWdr = true;
    stProfile.stBacklight.bSupportHlc = true;
    fill_user_percent_range(stProfile.stBacklight.stWdrLevel);
    fill_user_percent_range(stProfile.stBacklight.stHlcLevel);

    stProfile.stAwb.bSupported = true;
    stProfile.stAwb.vecSupportedModes = { ISP::AUTO_AWB_MODE, ISP::MANUAL_AWB_MODE,  ISP::LOCK_AWB_MODE, ISP::INCANDESCENT_MODE,
                                          ISP::WARM_MODE,     ISP::FLUORESCENT_MODE, ISP::DAY_LIGHT_MODE };
    fill_user_percent_range(stProfile.stAwb.stRGain);
    fill_user_percent_range(stProfile.stAwb.stBGain);

    stProfile.stNr.bSupported = true;
    stProfile.stNr.vecSupportedModes = { ISP::CLOSE_MODE, ISP::NORMAL_MODE, ISP::ADVANCED_MODE };
    fill_user_percent_range(stProfile.stNr.stDnrLevel);
    fill_user_percent_range(stProfile.stNr.stSnrLevel);
    fill_user_percent_range(stProfile.stNr.stTnrLevel);

    stProfile.stMirror.bSupported = true;
    stProfile.stMirror.vecSupportedModes = { ISP::DISABLE, ISP::HORIZONTAL, ISP::VERTICAL, ISP::CENTER };

    stProfile.stScene.bSupported = true;
    stProfile.stScene.bSupportSchedule = true;
    stProfile.stScene.vecSupportedScenes = { ISP::SCENE_NORMAL,   ISP::SCENE_FRONTLIGHT, ISP::SCENE_BACKLIGHT,
                                             ISP::SCENE_LOWLIGHT, ISP::SCENE_CUSTOM1,    ISP::SCENE_CUSTOM2 };

    /* 校验功能和参数范围是否自洽；失败时不启动服务。 */
    int nRet = IspCapabilityValidator_NS::validate_profile(stProfile);
    if (nRet != OK)
    {
        return nRet;
    }

    return OK;
}

} // namespace CapabilityBuilder_NS
