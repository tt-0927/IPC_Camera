/**
 * @FilePath     : isp_param_orchestrator.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:11:02
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-17 13:40:47
 * @Description  : 共享ISP参数编排器实现
 */

#include "isp_param_orchestrator.h"

#include <variant>

#include "IpcRet.h"
#include "dlog.h"
#include "isp_replay_order.h"

CIspParamOrchestrator::CIspParamOrchestrator(IIspConfigRepository &stRepository, IIspParameterApplier &stApplier)
    : m_rstRepository(stRepository), m_rstApplier(stApplier)
{
}

int CIspParamOrchestrator::apply_by_type(ISP::PicConfigureType_E enType)
{
    switch (enType)
    {
    case ISP::PicConfigureType_E::IAMGE:
    {
        ISP::IspConfigValue_T stValue = ISP::ImageParam_S{};
        int nRet = m_rstRepository.load(stValue);
        if (nRet != OK)
        {
            dlog_error("读取图像参数失败: %d", nRet);
            return nRet;
        }
        return m_rstApplier.apply_image(std::get<ISP::ImageParam_S>(stValue));
    }

    case ISP::PicConfigureType_E::EXPOSURE:
    {
        ISP::IspConfigValue_T stValue = ISP::ExposureAttr_S{};
        int nRet = m_rstRepository.load(stValue);
        if (nRet != OK)
        {
            dlog_error("读取曝光参数失败: %d", nRet);
            return nRet;
        }
        return m_rstApplier.apply_exposure(std::get<ISP::ExposureAttr_S>(stValue));
    }

    case ISP::PicConfigureType_E::BACKLIGHT:
    {
        ISP::IspConfigValue_T stValue = ISP::BackLightArrt_S{};
        int nRet = m_rstRepository.load(stValue);
        if (nRet != OK)
        {
            dlog_error("读取背光参数失败: %d", nRet);
            return nRet;
        }
        return m_rstApplier.apply_backlight(std::get<ISP::BackLightArrt_S>(stValue));
    }

    case ISP::PicConfigureType_E::AWB:
    {
        ISP::IspConfigValue_T stValue = ISP::AwbAttr_S{};
        int nRet = m_rstRepository.load(stValue);
        if (nRet != OK)
        {
            dlog_error("读取白平衡参数失败: %d", nRet);
            return nRet;
        }
        return m_rstApplier.apply_awb(std::get<ISP::AwbAttr_S>(stValue));
    }

    case ISP::PicConfigureType_E::NR:
    {
        ISP::IspConfigValue_T stValue = ISP::DnrAttr_S{};
        int nRet = m_rstRepository.load(stValue);
        if (nRet != OK)
        {
            dlog_error("读取降噪参数失败: %d", nRet);
            return nRet;
        }
        return m_rstApplier.apply_nr(std::get<ISP::DnrAttr_S>(stValue));
    }

    case ISP::PicConfigureType_E::MIRROR:
    {
        ISP::IspConfigValue_T stValue = ISP::VideoAdjust_S{};
        int nRet = m_rstRepository.load(stValue);
        if (nRet != OK)
        {
            dlog_error("读取镜像参数失败: %d", nRet);
            return nRet;
        }
        const ISP::VideoAdjust_S &stMirror = std::get<ISP::VideoAdjust_S>(stValue);
        /* info: 明确记录本次实际下发值，便于确认场景切换没有退回默认DISABLE。 */
        dlog_info("应用当前镜像配置, mode:%d", static_cast<int>(stMirror.enMirrorMode));
        return m_rstApplier.apply_mirror(stMirror);
    }

    case ISP::PicConfigureType_E::DAYNIGHT:
        dlog_warn("日夜配置不属于param业务域");
        return ERR_UNSUPPORT;

    case ISP::PicConfigureType_E::SCENE:
        dlog_warn("场景配置不属于param业务域");
        return ERR_UNSUPPORT;

    default:
        dlog_error("未知ISP基础参数类型: %d", static_cast<int>(enType));
        return ERR_PARAM;
    }
}

int CIspParamOrchestrator::apply_scene_param_by_type(ISP::PicConfigureType_E enType, const ISP::SceneParams_S &stSceneParams)
{
    switch (enType)
    {
    case ISP::PicConfigureType_E::IAMGE:
        return m_rstApplier.apply_image(stSceneParams.stImageParam);
    case ISP::PicConfigureType_E::EXPOSURE:
        return m_rstApplier.apply_exposure(stSceneParams.stExpAttr);
    case ISP::PicConfigureType_E::BACKLIGHT:
        return m_rstApplier.apply_backlight(stSceneParams.stBackLightAttr);
    case ISP::PicConfigureType_E::AWB:
        return m_rstApplier.apply_awb(stSceneParams.stAwbAttr);
    case ISP::PicConfigureType_E::NR:
        return m_rstApplier.apply_nr(stSceneParams.stDnrAttr);
    case ISP::PicConfigureType_E::MIRROR:
        /* 镜像跨配置场景共享，仍从独立存储读取当前全局值。 */
        return apply_by_type(enType);
    case ISP::PicConfigureType_E::DAYNIGHT:
    case ISP::PicConfigureType_E::SCENE:
        return ERR_UNSUPPORT;
    default:
        return ERR_PARAM;
    }
}

int CIspParamOrchestrator::replay_web_params(ISP::SceneType_E enConfigScene, ISP::IspRuntimeScene_E enRuntimeScene)
{
    /* 参数映射必须使用本次明确目标场景，禁止从尚未同步的detector状态反推。 */
    int nRet = m_rstApplier.set_runtime_scene_context(enRuntimeScene);
    if (nRet != OK)
    {
        dlog_error("设置ISP参数运行场景上下文失败, runtime_scene:%d, ret:%d", static_cast<int>(enRuntimeScene), nRet);
        return nRet;
    }

    /* 一次读取明确的场景槽位，避免计划场景与持久化用户场景不一致时重放错误参数。 */
    ISP::SceneParams_S stSceneParams;
    nRet = m_rstRepository.load_scene_params(enConfigScene, stSceneParams);
    if (nRet != OK)
    {
        dlog_error("读取场景重放参数失败, config_scene:%d, ret:%d", static_cast<int>(enConfigScene), nRet);
        return nRet;
    }

    /* step: 按固定顺序重放网页参数，中间失败立即停止。 */
    for (const ISP::PicConfigureType_E enType : IspReplayOrder_NS::scene_replay_order())
    {
        nRet = apply_scene_param_by_type(enType, stSceneParams);
        if (nRet != OK)
        {
            dlog_error("重放ISP网页参数失败, config_scene:%d, type:%d, ret:%d",
                       static_cast<int>(enConfigScene),
                       static_cast<int>(enType),
                       nRet);
            return nRet;
        }
    }

    /* 平台后处理钩子（如Gamma），由adapter内部决定是否执行。 */
    return m_rstApplier.on_scene_applied(enRuntimeScene);
}
