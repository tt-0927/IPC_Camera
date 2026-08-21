/**
 * @FilePath     : isp_runtime_arbiter.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-13 14:36:24
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-22 10:20:06
 * @Description  : ISP设置优先级选择器实现
 */

#include "isp_runtime_arbiter.h"

#include "IpcRet.h"
#include "dlog.h"
#include "fill_light_gate_policy.h"

namespace
{
/**
 * @brief   : 判断两个灯光目标是否相等
 * @param    {const ISP::IspLightTarget_S&} stLeft：左侧灯光目标
 * @param    {const ISP::IspLightTarget_S&} stRight：右侧灯光目标
 * @return   {bool} true：相等，false：不相等
 */
bool light_target_equal(const ISP::IspLightTarget_S &stLeft, const ISP::IspLightTarget_S &stRight)
{
    return stLeft.enLightType == stRight.enLightType && stLeft.nLightLevel == stRight.nLightLevel &&
           stLeft.bFlashing == stRight.bFlashing && stLeft.nFlashTimeSec == stRight.nFlashTimeSec &&
           stLeft.enFlashFrequency == stRight.enFlashFrequency;
}

/**
 * @brief   : 获取ISP补光类型的可读名称
 * @param    {ISP::LightType_E} enLightType：ISP补光类型
 * @return   {const char*} 用于日志的中文灯光名称
 */
const char *get_light_type_name(ISP::LightType_E enLightType)
{
    switch (enLightType)
    {
    case ISP::LIGHT_TYPE_WHITE:
    case ISP::LIGHT_TYPE_WHITE_ON_RED_OFF:
        return "白光";
    case ISP::LIGHT_TYPE_RED:
    case ISP::LIGHT_TYPE_RED_ON_WHITE_OFF:
        return "红外光";
    case ISP::LIGHT_TYPE_SMART:
        return "智能补光";
    case ISP::LIGHT_TYPE_CLOSE:
        return "关闭";
    case ISP::LIGHT_TYPE_BOTH:
        return "白光和红外光";
    default:
        return "未知灯光";
    }
}

/**
 * @brief   : 获取补光总控阻断原因的可读名称
 * @param    {Peripheral_NS::FillLightBlockReason_E} enReason：补光总控阻断原因
 * @return   {const char*} 用于日志的中文原因
 */
const char *get_fill_light_block_reason_name(Peripheral_NS::FillLightBlockReason_E enReason)
{
    switch (enReason)
    {
    case Peripheral_NS::FillLightBlockReason_E::NONE:
        return "无";
    case Peripheral_NS::FillLightBlockReason_E::DISABLED:
        return "外设补光总开关关闭";
    case Peripheral_NS::FillLightBlockReason_E::OUTSIDE_TIME_RANGE:
        return "定时模式当前不在生效时间内";
    case Peripheral_NS::FillLightBlockReason_E::ZERO_POWER_LIMIT:
        return "外设补光功率上限为0";
    default:
        return "未知原因";
    }
}
} // namespace

CIspRuntimeArbiter::CIspRuntimeArbiter() : m_u64NextToken(0)
{
}

void CIspRuntimeArbiter::update_user_scene(ISP::SceneType_E enScene)
{
    /* lock: 单次更新和重新选择必须原子，避免读到混合来源的硬件设置。 */
    std::lock_guard<std::mutex> stLock(m_mtx);
    m_stUserScene.enConfigScene = enScene;
    m_stUserScene.bActive = true;
    resolve_target_locked();
}

void CIspRuntimeArbiter::update_schedule_scene(ISP::SceneType_E enScene, bool bActive)
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    m_stSchedule.enConfigScene = enScene;
    m_stSchedule.bActive = bActive;
    resolve_target_locked();
}

void CIspRuntimeArbiter::clear_schedule_scene()
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    m_stSchedule.bActive = false;
    resolve_target_locked();
}

void CIspRuntimeArbiter::get_schedule_scene(ISP::SceneType_E &enScene, bool &bActive) const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    enScene = m_stSchedule.enConfigScene;
    bActive = m_stSchedule.bActive;
}

void CIspRuntimeArbiter::update_daynight(const ISP::IspDayNightIntent_S &stIntent)
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    m_stDayNight = stIntent;
    resolve_target_locked();
}

void CIspRuntimeArbiter::clear_daynight()
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    m_stDayNight.bActive = false;
    resolve_target_locked();
}

void CIspRuntimeArbiter::update_fill_light_gate(const Peripheral_NS::FillLightGateState_S &stGate)
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    m_stFillLightGate.stGate = stGate;
    m_stFillLightGate.bActive = true;
    /* 总开关在闪烁期间禁止补光时永久撤销本次临时灯光，之后重新允许也不能补闪。 */
    if (!stGate.bAllowed)
    {
        m_stOverride.bActive = false;
    }
    resolve_target_locked();
}

int CIspRuntimeArbiter::begin_light_override(const ISP::IspLightOverride_S &stOverride, uint64_t &u64Token)
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    u64Token = 0;
    /* ! 总开关禁止补光时不保存临时灯光，避免时间范围开始后执行过期告警。 */
    if (!m_stFillLightGate.bActive || !m_stFillLightGate.stGate.bAllowed)
    {
        return ERR_NOT_ENABLED;
    }
    if (m_stOverride.bActive)
    {
        return ERR;
    }
    m_stOverride = stOverride;
    /* 申请编号只在本类递增分配，防止过期调用误释放后来的临时灯光。 */
    m_stOverride.u64Token = ++m_u64NextToken;
    m_stOverride.bActive = true;
    resolve_target_locked();
    u64Token = m_stOverride.u64Token;
    return OK;
}

bool CIspRuntimeArbiter::end_light_override(uint64_t u64Token)
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    if (u64Token == 0 || m_stOverride.u64Token != u64Token)
    {
        return false;
    }
    /* 总开关可能已提前撤销此编号；重复清理仍成功，避免将正常清理报为参数错误。 */
    if (!m_stOverride.bActive)
    {
        return true;
    }
    m_stOverride.bActive = false;
    resolve_target_locked();
    return true;
}

bool CIspRuntimeArbiter::is_light_override_active(uint64_t u64Token) const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return u64Token != 0 && m_stOverride.bActive && m_stOverride.u64Token == u64Token;
}

bool CIspRuntimeArbiter::clear_expired_overrides(int64_t nCurrentTimeMs)
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    if (!m_stOverride.bActive || m_stOverride.nDeadlineMs <= 0)
    {
        return false;
    }
    if (nCurrentTimeMs < m_stOverride.nDeadlineMs)
    {
        return false;
    }
    m_stOverride.bActive = false;
    resolve_target_locked();
    return true;
}

ISP::IspRuntimeTarget_S CIspRuntimeArbiter::get_current_target() const
{
    std::lock_guard<std::mutex> stLock(m_mtx);
    return m_stCurrentTarget;
}

void CIspRuntimeArbiter::resolve_target_locked()
{
    /* stNewTarget 从默认值重建，避免上次目标中已经失效的可选字段被保留。 */
    ISP::IspRuntimeTarget_S stNewTarget;
    stNewTarget.u64Generation = m_stCurrentTarget.u64Generation;

    /* 用户场景只决定网页参数槽位，不参与日夜运行场景裁决。 */
    stNewTarget.enConfigScene = ISP::SCENE_NORMAL;
    if (m_stUserScene.bActive)
    {
        stNewTarget.enConfigScene = m_stUserScene.enConfigScene;
    }

    /* 日夜请求独立决定平台日夜场景、IR-CUT 和正常灯光。 */
    stNewTarget.enRuntimeScene = ISP::IspRuntimeScene_E::DAY;
    stNewTarget.enIrCutTarget = ISP::IspIrCutTarget_E::NONE;
    if (m_stDayNight.bActive)
    {
        stNewTarget.enRuntimeScene = m_stDayNight.enRuntimeScene;
        stNewTarget.enIrCutTarget = m_stDayNight.enIrCutTarget;
        stNewTarget.stLight = m_stDayNight.stLight;
    }

    /* 计划只替换网页配置场景，不能替换日夜硬件场景。 */
    if (m_stSchedule.bActive)
    {
        stNewTarget.enConfigScene = m_stSchedule.enConfigScene;
    }

    /* 优先级 2：临时灯光只替换灯光设置。 */
    if (m_stOverride.bActive)
    {
        stNewTarget.stLight = m_stOverride.stLight;
    }

    /* ! 总开关限制所有灯光请求，必须在临时灯光之后处理。 */
    if (!m_stFillLightGate.bActive || !m_stFillLightGate.stGate.bAllowed)
    {
        if (stNewTarget.stLight.enLightType != ISP::LIGHT_TYPE_CLOSE)
        {
            /* info: 记录被总控拒绝的原始请求，便于区分日夜策略选择灯型与外设最终禁灯。 */
            dlog_info("补光请求被外设总控限制为关闭, requested_light:%s(%d), requested_level:%u, reason:%s, power_limit:%u",
                      get_light_type_name(stNewTarget.stLight.enLightType),
                      static_cast<int>(stNewTarget.stLight.enLightType),
                      stNewTarget.stLight.nLightLevel,
                      m_stFillLightGate.bActive ? get_fill_light_block_reason_name(m_stFillLightGate.stGate.enBlockReason)
                                                : "补光总控状态未初始化",
                      m_stFillLightGate.bActive ? m_stFillLightGate.stGate.nPowerLimitPercent : 0U);
        }
        stNewTarget.stLight = ISP::IspLightTarget_S{};
    }
    else if (stNewTarget.stLight.enLightType != ISP::LIGHT_TYPE_CLOSE)
    {
        stNewTarget.stLight.nLightLevel = FillLightPolicy_NS::calculate_output_level(stNewTarget.stLight.nLightLevel,
                                                                                     m_stFillLightGate.stGate.nPowerLimitPercent);
        if (stNewTarget.stLight.nLightLevel == 0U)
        {
            stNewTarget.stLight = ISP::IspLightTarget_S{};
        }
    }

    /* 本类只判断待应用设置是否变化；硬件是否要操作由执行器比较最后成功状态。 */
    const bool bNeedConfigReplay = (stNewTarget.enConfigScene != m_stCurrentTarget.enConfigScene);
    const bool bNeedRuntimeSceneApply = (stNewTarget.enRuntimeScene != m_stCurrentTarget.enRuntimeScene);
    const bool bIrCutTargetChanged = (stNewTarget.enIrCutTarget != m_stCurrentTarget.enIrCutTarget);
    const bool bLightTargetChanged = !light_target_equal(stNewTarget.stLight, m_stCurrentTarget.stLight);

    /* 设置变化时更新序号（generation）加一；无变化不增加。 */
    if (bNeedConfigReplay || bNeedRuntimeSceneApply || bIrCutTargetChanged || bLightTargetChanged)
    {
        stNewTarget.u64Generation = m_stCurrentTarget.u64Generation + 1;
    }

    m_stCurrentTarget = stNewTarget;
}
