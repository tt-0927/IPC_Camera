/**
 * @FilePath     : fill_light_gate_controller.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-07-17 11:39:41
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-20 15:34:05
 * @Description  : 外设补光一级总控配置快照、定时检查与安全通知控制器实现
 */

#include "fill_light_gate_controller.h"

#include <chrono>
#include <system_error>

#include "IpcRet.h"
#include "dlog.h"
#include "fill_light_gate_policy.h"
#include "time_utils.h"

namespace
{
/* gate定时复核周期，保证手动校时后最多一秒更新。 */
constexpr std::chrono::seconds GATE_REFRESH_INTERVAL(1);

/**
 * @brief   : 判断两组gate是否具有相同运行语义
 * @param    {const Peripheral_NS::FillLightGateState_S&} stLeft：左侧gate
 * @param    {const Peripheral_NS::FillLightGateState_S&} stRight：右侧gate
 * @return   {bool} true：相同，false：不同
 */
bool gate_equal(const Peripheral_NS::FillLightGateState_S &stLeft, const Peripheral_NS::FillLightGateState_S &stRight)
{
    return stLeft.bAllowed == stRight.bAllowed && stLeft.nPowerLimitPercent == stRight.nPowerLimitPercent &&
           stLeft.enBlockReason == stRight.enBlockReason;
}

/**
 * @brief   : 获取补光gate阻断原因的可读名称
 * @param    {Peripheral_NS::FillLightBlockReason_E} enReason：补光gate阻断原因
 * @return   {const char*} 用于日志的中文原因
 */
const char *get_gate_block_reason_name(Peripheral_NS::FillLightBlockReason_E enReason)
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

CFillLightGateController::CFillLightGateController()
    : m_pstSink(nullptr), m_bInitialized(false), m_bStopRequested(false), m_u64GateVersion(0), m_u64NotifiedGateVersion(0),
      m_nLastNotifyError(OK)
{
}

CFillLightGateController::~CFillLightGateController()
{
    deinit();
}

int CFillLightGateController::init(const Peripheral_NS::FillLightGlobalConfig_S &stConfig)
{
    /* 初始化即裁决一次，避免worker首次轮询前出现未受总控约束的输出窗口。 */
    const Peripheral_NS::FillLightGateState_S stInitialGate = FillLightPolicy_NS::evaluate_gate(
        stConfig,
        TimeUtils_NS::getSecondsSinceStartOfDay());
    std::uint64_t u64InitialVersion = 0;

    {
        std::lock_guard<std::mutex> stLock(m_mtxState);
        if (m_bInitialized)
        {
            return OK;
        }

        /* 先固化配置与对应gate，再以同一版本通知下游，保证快照一致。 */
        m_stConfig = stConfig;
        m_stGate = stInitialGate;
        m_bStopRequested = false;
        m_nLastNotifyError = OK;
        m_bInitialized = true;
        u64InitialVersion = ++m_u64GateVersion;
    }

    try
    {
        /* memory: worker由本对象独占，任何失败路径都恢复初始化状态。 */
        m_stWorker = std::thread(&CFillLightGateController::worker_loop, this);
    }
    catch (const std::system_error &stError)
    {
        std::lock_guard<std::mutex> stLock(m_mtxState);
        m_bInitialized = false;
        m_bStopRequested = true;
        dlog_error("补光gate worker启动失败, code:%d, message:%s", stError.code().value(), stError.what());
        return ERR;
    }

    /* worker启动后立即重放初始gate，使已注册的ISP适配层同步进入受控状态。 */
    const int nRet = notify_gate(u64InitialVersion);
    if (nRet != OK)
    {
        deinit();
        return nRet;
    }

    dlog_info("外设补光gate控制器初始化完成, allowed:%d, limit:%u, reason:%d",
              stInitialGate.bAllowed,
              stInitialGate.nPowerLimitPercent,
              static_cast<int>(stInitialGate.enBlockReason));
    return OK;
}

int CFillLightGateController::deinit()
{
    {
        std::lock_guard<std::mutex> stLock(m_mtxState);
        if (!m_bInitialized && !m_stWorker.joinable())
        {
            m_pstSink = nullptr;
            return OK;
        }
        /* 请求退出后唤醒定时等待，随后join保证不再并发访问控制器状态。 */
        m_bStopRequested = true;
    }
    m_stWakeCv.notify_all();

    if (m_stWorker.joinable())
    {
        m_stWorker.join();
    }

    /* lock: 等待可能正在执行的sink回调结束，再释放非拥有指针。 */
    std::lock_guard<std::mutex> stCallbackLock(m_mtxCallback);
    std::lock_guard<std::mutex> stStateLock(m_mtxState);
    /* 回调已完全退出后才解除非拥有sink指针，避免析构期间发生悬空回调。 */
    m_pstSink = nullptr;
    m_bInitialized = false;
    m_bStopRequested = false;
    m_nLastNotifyError = OK;
    dlog_info("外设补光gate控制器已停止");
    return OK;
}

int CFillLightGateController::update_config(const Peripheral_NS::FillLightGlobalConfig_S &stConfig)
{
    /* 读取一次当前时间，使本次配置快照和定时准入判断使用同一时间基准。 */
    const int nNowSecOfDay = TimeUtils_NS::getSecondsSinceStartOfDay();
    std::uint64_t u64Version = 0;
    bool bGateChanged = false;
    Peripheral_NS::FillLightGateState_S stOldGate;
    Peripheral_NS::FillLightGateState_S stNewGate;

    {
        std::lock_guard<std::mutex> stLock(m_mtxState);
        if (!m_bInitialized)
        {
            return ERR_UNINIT;
        }

        /* 特殊局部变量保存新裁决结果，只有运行语义变化才递增版本并触发下游。 */
        stNewGate = FillLightPolicy_NS::evaluate_gate(stConfig, nNowSecOfDay);
        bGateChanged = !gate_equal(m_stGate, stNewGate);
        stOldGate = m_stGate;
        m_stConfig = stConfig;
        if (bGateChanged)
        {
            m_stGate = stNewGate;
            u64Version = ++m_u64GateVersion;
        }
    }
    /* 新定时范围可能改变下一次状态翻转时刻，唤醒worker立即复核。 */
    m_stWakeCv.notify_all();

    if (bGateChanged)
    {
        dlog_info("外设补光配置触发gate变化, now_sec:%d, old_allowed:%d, new_allowed:%d, old_reason:%s, "
                  "new_reason:%s, old_limit:%u, new_limit:%u",
                  nNowSecOfDay,
                  stOldGate.bAllowed,
                  stNewGate.bAllowed,
                  get_gate_block_reason_name(stOldGate.enBlockReason),
                  get_gate_block_reason_name(stNewGate.enBlockReason),
                  stOldGate.nPowerLimitPercent,
                  stNewGate.nPowerLimitPercent);
    }

    if (!bGateChanged)
    {
        std::lock_guard<std::mutex> stLock(m_mtxState);
        if (m_u64NotifiedGateVersion == m_u64GateVersion)
        {
            return OK;
        }
        u64Version = m_u64GateVersion;
    }

    /* 配置未改变gate时，仍补发此前失败的版本，避免持久化成功而硬件未收敛。 */
    return notify_gate(u64Version);
}

int CFillLightGateController::refresh_after_time_change(const char *pszTrigger)
{
    /* 空来源不影响策略，仅使用稳定兜底文本保证日志字段可读。 */
    return refresh_gate(pszTrigger == nullptr ? "系统时间变化" : pszTrigger, true);
}

int CFillLightGateController::set_sink(IFillLightGateSink *pSink)
{
    if (pSink == nullptr)
    {
        return ERR_PARAM_NULL;
    }

    /* lock: 注册和首次重放与worker通知串行，避免新sink先收到旧版本。 */
    std::lock_guard<std::mutex> stCallbackLock(m_mtxCallback);
    Peripheral_NS::FillLightGateState_S stGate;
    std::uint64_t u64Version = 0;
    bool bNeedReplay = false;
    {
        std::lock_guard<std::mutex> stStateLock(m_mtxState);
        if (m_pstSink != nullptr && m_pstSink != pSink)
        {
            return ERR;
        }
        /* 在状态锁内完成注册和版本快照，回放时不会误确认后续的新版本。 */
        m_pstSink = pSink;
        bNeedReplay = m_bInitialized;
        stGate = m_stGate;
        u64Version = m_u64GateVersion;
    }

    if (!bNeedReplay)
    {
        return OK;
    }

    /* 首次注册必须同步当前总控，sink不得假定后续一定会有配置变更通知。 */
    const int nRet = pSink->update_fill_light_gate(stGate);
    if (nRet != OK)
    {
        std::lock_guard<std::mutex> stStateLock(m_mtxState);
        if (m_pstSink == pSink)
        {
            m_pstSink = nullptr;
        }
    }
    else
    {
        std::lock_guard<std::mutex> stStateLock(m_mtxState);
        if (m_pstSink == pSink && m_u64GateVersion == u64Version)
        {
            m_u64NotifiedGateVersion = u64Version;
        }
    }
    return nRet;
}

int CFillLightGateController::clear_sink(IFillLightGateSink *pSink)
{
    if (pSink == nullptr)
    {
        return ERR_PARAM_NULL;
    }

    /* lock: 回调mutex是注销屏障，取得后保证旧sink没有正在执行的回调。 */
    std::lock_guard<std::mutex> stCallbackLock(m_mtxCallback);
    std::lock_guard<std::mutex> stStateLock(m_mtxState);
    if (m_pstSink != pSink)
    {
        return ERR_PARAM;
    }
    m_pstSink = nullptr;
    return OK;
}

int CFillLightGateController::get_gate(Peripheral_NS::FillLightGateState_S &stGate) const
{
    std::lock_guard<std::mutex> stLock(m_mtxState);
    if (!m_bInitialized)
    {
        return ERR_UNINIT;
    }
    stGate = m_stGate;
    return OK;
}

void CFillLightGateController::worker_loop()
{
    /* worker只负责时间边界复核；实际下游回调在notify_gate中通过独立锁串行执行。 */
    while (true)
    {
        std::unique_lock<std::mutex> stLock(m_mtxState);
        m_stWakeCv.wait_for(stLock,
                            GATE_REFRESH_INTERVAL,
                            [this]
                            {
                                return m_bStopRequested;
                            });
        if (m_bStopRequested)
        {
            return;
        }
        stLock.unlock();

        /* 状态锁外执行策略与回调，避免外部sink耗时阻塞配置读写。 */
        const int nRet = refresh_gate("周期复核", false);
        if (nRet != OK && nRet != m_nLastNotifyError)
        {
            dlog_warn("外设补光gate定时通知失败: %d", nRet);
        }
        else if (nRet == OK && m_nLastNotifyError != OK)
        {
            dlog_info("外设补光gate定时通知已恢复, previous_ret:%d", m_nLastNotifyError);
        }
        m_nLastNotifyError = nRet;
    }
}

int CFillLightGateController::refresh_gate(const char *pszTrigger, bool bLogUnchanged)
{
    /* 每轮重新取时，兼容系统校时以及跨午夜时间窗。 */
    const int nNowSecOfDay = TimeUtils_NS::getSecondsSinceStartOfDay();
    std::uint64_t u64Version = 0;
    bool bGateChanged = false;
    bool bAlreadyNotified = false;
    Peripheral_NS::FillLightGateState_S stOldGate;
    Peripheral_NS::FillLightGateState_S stNewGate;

    {
        std::lock_guard<std::mutex> stLock(m_mtxState);
        if (!m_bInitialized)
        {
            return ERR_UNINIT;
        }

        /* 仅以可输出性、功率上限和阻断原因决定版本，避免无效通知。 */
        stNewGate = FillLightPolicy_NS::evaluate_gate(m_stConfig, nNowSecOfDay);
        if (gate_equal(m_stGate, stNewGate))
        {
            if (m_u64NotifiedGateVersion == m_u64GateVersion)
            {
                bAlreadyNotified = true;
            }
            else
            {
                u64Version = m_u64GateVersion;
            }
        }
        else
        {
            stOldGate = m_stGate;
            m_stGate = stNewGate;
            u64Version = ++m_u64GateVersion;
            bGateChanged = true;
        }
    }

    if (bGateChanged)
    {
        dlog_info("系统时间触发补光gate变化, trigger:%s, now_sec:%d, old_allowed:%d, new_allowed:%d, "
                  "old_reason:%s, new_reason:%s, old_limit:%u, new_limit:%u",
                  pszTrigger,
                  nNowSecOfDay,
                  stOldGate.bAllowed,
                  stNewGate.bAllowed,
                  get_gate_block_reason_name(stOldGate.enBlockReason),
                  get_gate_block_reason_name(stNewGate.enBlockReason),
                  stOldGate.nPowerLimitPercent,
                  stNewGate.nPowerLimitPercent);
    }
    else if (bLogUnchanged)
    {
        dlog_info("系统时间触发补光gate重算无变化, trigger:%s, now_sec:%d, allowed:%d, reason:%s, limit:%u",
                  pszTrigger,
                  nNowSecOfDay,
                  stNewGate.bAllowed,
                  get_gate_block_reason_name(stNewGate.enBlockReason),
                  stNewGate.nPowerLimitPercent);
    }

    if (bAlreadyNotified)
    {
        return OK;
    }

    return notify_gate(u64Version);
}

int CFillLightGateController::notify_gate(std::uint64_t u64ExpectedVersion)
{
    /* lock: 串行回调，并确保clear_sink可等待回调完整返回。 */
    std::lock_guard<std::mutex> stCallbackLock(m_mtxCallback);
    IFillLightGateSink *pSink = nullptr;
    Peripheral_NS::FillLightGateState_S stGate;
    {
        std::lock_guard<std::mutex> stStateLock(m_mtxState);
        /* 版本不一致表示已有更新，本次旧快照不得覆盖新gate。 */
        if (!m_bInitialized || u64ExpectedVersion != m_u64GateVersion || m_pstSink == nullptr)
        {
            return OK;
        }
        pSink = m_pstSink;
        stGate = m_stGate;
    }

    /* 回调在状态锁外执行，允许sink回调ISP/硬件而不会形成锁递归。 */
    const int nRet = pSink->update_fill_light_gate(stGate);
    if (nRet == OK)
    {
        std::lock_guard<std::mutex> stStateLock(m_mtxState);
        if (m_pstSink == pSink && m_u64GateVersion == u64ExpectedVersion)
        {
            m_u64NotifiedGateVersion = u64ExpectedVersion;
        }
    }
    return nRet;
}
