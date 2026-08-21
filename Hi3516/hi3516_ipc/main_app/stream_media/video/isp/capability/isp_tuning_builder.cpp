/**
 * @FilePath     : isp_tuning_builder.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-10 15:17:16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-27 10:55:46
 * @Description  : Hi3516 ISP配置画像构建与Sensor静态资源解析实现
 */

#include "isp_tuning_builder.h"

#include "IpcRet.h"
#include "dlog.h"
#include "isp_profile_config_loader.h"
#include "sample_comm.h"

#ifdef SENSOR_SC533HAI
#include "sc533hai_cmos.h"
#endif

namespace
{
/**
 * @brief   : 获取ISP启动传感器类型
 * @return   {int} sample_sns_type对应的传感器类型值
 * @note    : Sensor驱动在编译期链接，不能由运行时配置切换
 */
int resolve_sensor_type()
{
#ifdef SENSOR_SC533HAI
    return SC533HAI_MIPI_5M_30FPS_10BIT;
#elif defined(SENSOR_SC500AI)
    return SC500AI_MIPI_5M_30FPS_10BIT;
#else
    return SC500AI_MIPI_5M_30FPS_10BIT;
#endif
}

/**
 * @brief   : 根据运行策略绑定Sensor静态Gamma资源
 * @param    {Hi3516TuningProfile_S &} stProfile：待绑定画像
 * @return   {int} OK：成功，ERR_UNSUPPORT：当前Sensor不提供目标资源
 */
int resolve_gamma_resources(Hi3516TuningProfile_S &stProfile)
{
    if (!stProfile.bUseCmosGamma)
    {
        stProfile.pGammaDay = nullptr;
        stProfile.pGammaIr = nullptr;
        return OK;
    }

#ifdef SENSOR_SC533HAI
    stProfile.pGammaDay = sc533hai_get_cmos_gamma();
    stProfile.pGammaIr = sc533hai_get_cmos_gamma_ir();
    if (stProfile.pGammaDay == nullptr || stProfile.pGammaIr == nullptr)
    {
        dlog_error("SC533HAI Gamma静态资源为空");
        return ERR_NOT_EXIST;
    }
    return OK;
#else
    dlog_error("当前Sensor不支持配置要求的CMOS Gamma资源");
    return ERR_UNSUPPORT;
#endif
}
} // namespace

namespace IspTuningBuilder_NS
{
int build_tuning_profile(const std::string &strConfigDir, Hi3516TuningProfile_S &stProfile)
{
    Hi3516TuningProfile_S stLoadedProfile;
    int nRet = IspProfileConfigLoader_NS::load(strConfigDir, SENSOR_TYPE_STR, DEVICE_TYPE_STR, stLoadedProfile);
    if (nRet != OK)
    {
        return nRet;
    }

    stLoadedProfile.nSensorType = resolve_sensor_type();
    nRet = resolve_gamma_resources(stLoadedProfile);
    if (nRet != OK)
    {
        return nRet;
    }

    stProfile = stLoadedProfile;
    return OK;
}

int get_sensor_type()
{
    return resolve_sensor_type();
}
} // namespace IspTuningBuilder_NS
