/**
 * @FilePath     : isp_capability_builder.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-22 15:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-11 13:51:17
 * @Description  : RV1126B ISP功能和参数范围构建实现
 */

#include "isp_capability_builder.h"

#include "IpcRet.h"
#include "isp_capability_validator.h"
#include "isp_define.h"

namespace
{
constexpr int ISP_USER_PARAM_MIN = 0;
constexpr int ISP_USER_PARAM_MAX = 100;
constexpr int ISP_USER_PARAM_STEP = 1;
constexpr int ISP_USER_PARAM_DEFAULT = 50;
constexpr int DAY_NIGHT_SENSITIVITY_MIN = 1;
constexpr int DAY_NIGHT_SENSITIVITY_MAX = 10;
/* 保持DayNightAttr_S与RV1126B历史配置的默认等级，SmartIR启动前会由共享层显式同步。 */
constexpr int DAY_NIGHT_SENSITIVITY_DEFAULT = 2;
constexpr int DAY_NIGHT_FILTER_TIME_DEFAULT = 15;

/**
 * @brief   : 填充一个已支持的整数范围
 * @param    {ISP::IspParamRange_S&} stRange：待填充范围
 * @param    {int} nMin：最小值
 * @param    {int} nMax：最大值
 * @param    {int} nStep：步进值
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
 * @brief   : 填充网页百分比参数范围
 * @param    {ISP::IspParamRange_S&} stRange：待填充范围
 * @return   {void}
 */
void fill_user_percent_range(ISP::IspParamRange_S &stRange)
{
    fill_supported_range(stRange, ISP_USER_PARAM_MIN, ISP_USER_PARAM_MAX, ISP_USER_PARAM_STEP, ISP_USER_PARAM_DEFAULT);
}
} // namespace

namespace Rv1126bIspCapabilityBuilder_NS
{
int build_profile(ISP::IspCapabilityProfile_S &stProfile)
{
    /* 清空输出，失败重试时不能沿用上一轮更宽的能力画像。 */
    stProfile = ISP::IspCapabilityProfile_S();
#if !CAP_LIGHT_WHITE_ONLY || CAP_ISP_IR_SWITCH
    /* ! 当前RV1126B适配只允许白光-only画像；宏变化时必须先补齐红外/IR-CUT平台实现。 */
    return ERR_UNSUPPORT;
#else
    /* RV1126B两款产品均只有白光灯，不得把红外或IR-CUT能力泄漏给共享策略。 */
    stProfile.bSupportIrCut = false;
    stProfile.bWhiteOnly = true;
    stProfile.bSupportRedLight = false;

    fill_user_percent_range(stProfile.stBrightness);
    fill_user_percent_range(stProfile.stContrast);
    fill_user_percent_range(stProfile.stSaturation);
    fill_user_percent_range(stProfile.stSharpness);
    fill_user_percent_range(stProfile.stWhiteLightLevel);
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

    /* RV1126B夜间始终保持全彩IQ；只允许共享状态机输出白光或关灯运行场景。 */
    stProfile.vecSupportedRuntimeScenes = {
        ISP::IspRuntimeScene_E::DAY,
        ISP::IspRuntimeScene_E::NIGHT_WHITE,
        ISP::IspRuntimeScene_E::NIGHT_LIGHT_OFF,
    };

    /* info: 仅声明RK AIQ当前已实现的曝光、背光、AWB、降噪和镜像范围。 */
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

    return IspCapabilityValidator_NS::validate_profile(stProfile);
#endif
}
} // namespace Rv1126bIspCapabilityBuilder_NS
