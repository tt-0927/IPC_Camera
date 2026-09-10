/**
 * @FilePath     : isp_config_command_service.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 12:24:26
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-20 17:48:54
 * @Description  : ISP固定命令配置应用事务服务实现
 */

#include "isp_config_command_service.h"

#include <variant>
#include <type_traits>

#include "IpcRet.h"
#include "dlog.h"
#include "isp_param_policy.h"
#include "isp_scene_schedule_policy.h"

CIspConfigCommandService::CIspConfigCommandService(IIspConfigRepository &stRepository,
                                                   ISP::IIspBusinessService &stBusinessService)
    : m_rstRepository(stRepository), m_rstBusinessService(stBusinessService)
{
}

int CIspConfigCommandService::set_image_config(const ISP::ImageParam_S &stConfig)
{
    return set_config(ISP::IspConfigValue_T{ stConfig });
}

int CIspConfigCommandService::set_exposure_config(const ISP::ExposureAttr_S &stConfig)
{
    return set_config(ISP::IspConfigValue_T{ stConfig });
}

int CIspConfigCommandService::set_daynight_config(const ISP::DayNightAttr_S &stConfig)
{
    return set_config(ISP::IspConfigValue_T{ stConfig });
}

int CIspConfigCommandService::set_backlight_config(const ISP::BackLightArrt_S &stConfig)
{
    return set_config(ISP::IspConfigValue_T{ stConfig });
}

int CIspConfigCommandService::set_awb_config(const ISP::AwbAttr_S &stConfig)
{
    return set_config(ISP::IspConfigValue_T{ stConfig });
}

int CIspConfigCommandService::set_nr_config(const ISP::DnrAttr_S &stConfig)
{
    return set_config(ISP::IspConfigValue_T{ stConfig });
}

int CIspConfigCommandService::set_mirror_config(const ISP::VideoAdjust_S &stConfig)
{
    return set_config(ISP::IspConfigValue_T{ stConfig });
}

int CIspConfigCommandService::set_scene_schedule(const ISP::SceneSchedule_S &stConfig)
{
    return set_config(ISP::IspConfigValue_T{ stConfig });
}

int CIspConfigCommandService::set_user_scene(ISP::SceneType_E enScene)
{
    return set_config(ISP::IspConfigValue_T{ enScene });
}

int CIspConfigCommandService::set_config(ISP::IspConfigValue_T stNewValue)
{
    /* step1: 获取能力画像，校验/归一化必须在持久化前完成。 */
    ISP::IspCapabilityProfile_S stProfile;
    int nRet = m_rstBusinessService.get_capability_profile(stProfile);
    if (nRet != OK)
    {
        dlog_error("ISP命令事务获取能力画像失败: %d", nRet);
        return nRet;
    }

    nRet = normalize(stNewValue, stProfile);
    if (nRet != OK)
    {
        return nRet;
    }

    /* step2: 新值的备选类型同时充当读取类型标签，保证旧快照与新值可互换恢复。 */
    ISP::IspConfigValue_T stOldValue = stNewValue;
    nRet = m_rstRepository.load(stOldValue);
    if (nRet != OK)
    {
        dlog_error("ISP命令事务读取旧配置失败: %d", nRet);
        return nRet;
    }

    /* step3: 持久化新配置。 */
    nRet = m_rstRepository.save(stNewValue);
    if (nRet != OK)
    {
        dlog_error("ISP命令事务持久化失败: %d", nRet);
        return nRet;
    }

    /* step4: 提交应用，失败时恢复旧配置。 */
    nRet = m_rstBusinessService.apply_config(stNewValue);
    if (nRet != OK)
    {
        return restore_after_apply_failure(stOldValue, nRet);
    }

    return OK;
}

int CIspConfigCommandService::normalize(ISP::IspConfigValue_T &stValue, const ISP::IspCapabilityProfile_S &stProfile) const
{
    /* 按 variant 的具体配置域选择策略，策略可在原对象上裁剪默认值或范围。 */
    return std::visit(
        [&stProfile](auto &stConfig) -> int
        {
            /* T 是编译期类型标签，避免将不同配置结构交给错误的策略函数。 */
            using T = std::decay_t<decltype(stConfig)>;
            if constexpr (std::is_same_v<T, ISP::ImageParam_S>)
            {
                return IspParamPolicy_NS::normalize_image_param(stConfig, stProfile);
            }
            else if constexpr (std::is_same_v<T, ISP::ExposureAttr_S>)
            {
                return IspParamPolicy_NS::normalize_exposure(stConfig, stProfile);
            }
            else if constexpr (std::is_same_v<T, ISP::DayNightAttr_S>)
            {
                return IspParamPolicy_NS::normalize_daynight(stConfig, stProfile);
            }
            else if constexpr (std::is_same_v<T, ISP::BackLightArrt_S>)
            {
                return IspParamPolicy_NS::normalize_backlight(stConfig, stProfile);
            }
            else if constexpr (std::is_same_v<T, ISP::AwbAttr_S>)
            {
                return IspParamPolicy_NS::normalize_awb(stConfig, stProfile);
            }
            else if constexpr (std::is_same_v<T, ISP::DnrAttr_S>)
            {
                return IspParamPolicy_NS::normalize_nr(stConfig, stProfile);
            }
            else if constexpr (std::is_same_v<T, ISP::VideoAdjust_S>)
            {
                return IspParamPolicy_NS::normalize_mirror(stConfig, stProfile);
            }
            else if constexpr (std::is_same_v<T, ISP::SceneType_E>)
            {
                return IspParamPolicy_NS::normalize_scene(stConfig, stProfile);
            }
            else if constexpr (std::is_same_v<T, ISP::SceneSchedule_S>)
            {
                return IspSceneSchedulePolicy_NS::normalize_scene_schedule(stConfig, stProfile);
            }
        },
        stValue);
}

int CIspConfigCommandService::restore_after_apply_failure(const ISP::IspConfigValue_T &stOldValue, int nOriginalRet)
{
    /* ! 恢复失败只记录错误，不覆盖原始应用错误码。 */
    int nRestoreRet = m_rstRepository.save(stOldValue);
    if (nRestoreRet != OK)
    {
        dlog_error("ISP命令事务恢复旧配置持久化失败: %d, 原始错误: %d", nRestoreRet, nOriginalRet);
        return nOriginalRet;
    }

    nRestoreRet = m_rstBusinessService.apply_config(stOldValue);
    if (nRestoreRet != OK)
    {
        dlog_error("ISP命令事务恢复旧配置应用失败: %d, 原始错误: %d", nRestoreRet, nOriginalRet);
    }

    return nOriginalRet;
}

int CIspConfigCommandService::restore_default_config()
{
    /* step1: 读取旧快照，用于失败恢复。 */
    ISP::AllSceneParams_S stOldAllParams;
    int nRet = m_rstRepository.load_all_scene_params(stOldAllParams);
    if (nRet != OK)
    {
        dlog_error("恢复默认读取旧场景快照失败: %d", nRet);
        return nRet;
    }

    /* 镜像不随场景槽位保存，需单独快照以组成可回滚的恢复默认事务。 */
    ISP::VideoAdjust_S stOldMirror;
    ISP::IspConfigValue_T stOldMirrorVariant = stOldMirror;
    nRet = m_rstRepository.load(stOldMirrorVariant);
    if (nRet != OK || !std::holds_alternative<ISP::VideoAdjust_S>(stOldMirrorVariant))
    {
        dlog_error("恢复默认读取旧镜像快照失败: %d", nRet);
        return (nRet == OK) ? ERR : nRet;
    }
    stOldMirror = std::get<ISP::VideoAdjust_S>(stOldMirrorVariant);

    /* step2: 恢复当前场景槽默认配置。 */
    nRet = m_rstRepository.restore_defaults();
    if (nRet != OK)
    {
        dlog_error("恢复默认restore_defaults失败: %d", nRet);
        return restore_default_after_failure(stOldAllParams, stOldMirror, nRet);
    }

    /* step3: 保存默认镜像。 */
    /* 默认构造值与既有配置模型默认值保持一致，不能复用旧镜像快照。 */
    ISP::VideoAdjust_S stDefaultMirror;
    nRet = m_rstRepository.save(ISP::IspConfigValue_T{ stDefaultMirror });
    if (nRet != OK)
    {
        dlog_error("恢复默认保存默认镜像失败: %d", nRet);
        return restore_default_after_failure(stOldAllParams, stOldMirror, nRet);
    }

    /* step4: 全量重新应用硬件运行态。 */
    nRet = m_rstBusinessService.reconcile_all();
    if (nRet != OK)
    {
        dlog_error("恢复默认reconcile_all失败: %d, 尝试恢复旧快照", nRet);

        return restore_default_after_failure(stOldAllParams, stOldMirror, nRet);
    }

    return OK;
}

int CIspConfigCommandService::restore_default_after_failure(const ISP::AllSceneParams_S &stOldAllParams,
                                                            const ISP::VideoAdjust_S &stOldMirror,
                                                            int nOriginalRet)
{
    /* ! 组合事务按配置域逐项尽力回滚，任何回滚失败都不能覆盖最先发生的错误。 */
    int nRestoreRet = m_rstRepository.save_all_scene_params(stOldAllParams);
    if (nRestoreRet != OK)
    {
        dlog_error("恢复默认回滚场景快照失败: %d, 原始错误: %d", nRestoreRet, nOriginalRet);
    }

    nRestoreRet = m_rstRepository.save(ISP::IspConfigValue_T{ stOldMirror });
    if (nRestoreRet != OK)
    {
        dlog_error("恢复默认回滚镜像快照失败: %d, 原始错误: %d", nRestoreRet, nOriginalRet);
    }

    nRestoreRet = m_rstBusinessService.reconcile_all();
    if (nRestoreRet != OK)
    {
        dlog_error("恢复默认回滚后重应用运行态失败: %d, 原始错误: %d", nRestoreRet, nOriginalRet);
    }
    return nOriginalRet;
}
