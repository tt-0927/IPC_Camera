/**
 * @FilePath     : isp_dayNight.cpp
 * @Author       : cyc
 * @Date         : 2026-02-27 13:40:43
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-04 16:55:40
 * @Description  : 日夜自动检测控制器实现（仅自动观测）
 */

#include "isp_dayNight.h"

#include <exception>
#include <unistd.h>
#include <utility>

#include "IpcRet.h"
#include "dlog.h"
#include "os.h"
#include "ot_common_isp.h"
#include "ss_mpi_ae.h"
#include "ss_mpi_isp.h"

using namespace ISP;

namespace
{
constexpr uint32_t DAY_TO_NIGHT_THRESH_BY_SENSITIVITY[8] = {
    160000000, 155000000, 150000000, 125000000, 100000000, 80000000, 60000000, 40000000,
};
constexpr int DAY_TO_NIGHT_THRESH_COUNT = static_cast<int>(sizeof(DAY_TO_NIGHT_THRESH_BY_SENSITIVITY) /
                                                           sizeof(DAY_TO_NIGHT_THRESH_BY_SENSITIVITY[0]));

constexpr uint64_t DEFAULT_WHITE_TO_DAY_BRIGHT = 40000000;
constexpr uint64_t DEFAULT_RED_TO_DAY_BRIGHT = 10000000;
constexpr td_u32 DEFAULT_WHITE_RG = 160;
constexpr td_u32 DEFAULT_WHITE_BG = 185;
constexpr td_u32 DEFAULT_RED_RG = 240;
constexpr td_u32 DEFAULT_RED_BG = 240;
constexpr int SHIFT_8BIT = 8;
} // namespace

CDayNightController::CDayNightController()
{
    /* 初始化MPP红外自动检测基线；运行态同步线程写入，检测线程只复制后交给MPI接口。 */
    m_irAttr.enable = TD_TRUE;
    m_irAttr.normal_to_ir_iso_threshold = NORMAL_TO_IR_THRESHOLD;
    m_irAttr.ir_to_normal_iso_threshold = IR_TO_NORMAL_THRESHOLD;
    m_irAttr.rg_max = 200;
    m_irAttr.rg_min = 150;
    m_irAttr.bg_max = 130;
    m_irAttr.bg_min = 100;
    m_irAttr.ir_status = OT_ISP_IR_STATUS_NORMAL;
    m_nLastCollectErrorCode = OK;
}

CDayNightController::~CDayNightController()
{
    stop();
}

bool CDayNightController::start()
{
    if (m_running.load())
    {
        return true;
    }
    /* ! 已接受状态只能由sync_runtime_context更新；线程重启必须保留夜间基线和连续检测候选。 */
    m_running.store(true);
    m_enLastCollectErrorStage = CollectStage_E::NONE;
    m_nLastCollectErrorCode = OK;
    try
    {
        m_workerThread = std::make_unique<std::thread>(&CDayNightController::workerThread, this);
    }
    catch (const std::exception &stError)
    {
        m_running.store(false);
        m_workerThread.reset();
        dlog_error("Hi3516日夜检测线程启动失败: %s", stError.what());
        return false;
    }
    dlog_info("Hi3516日夜检测线程已启动, interval_ms:%lld, baseline:%s",
              static_cast<long long>(THREAD_SLEEP_INTERVAL.count()),
              m_isNight.load() ? "夜间" : "白天");
    return true;
}

void CDayNightController::stop()
{
    if (!m_running.load())
    {
        return;
    }
    /* ! 必须先发出退出条件再 join，避免析构阶段遗留访问成员的检测线程。 */
    m_running.store(false);
    if (m_workerThread && m_workerThread->joinable())
    {
        m_workerThread->join();
    }
    m_workerThread.reset();
    dlog_info("Hi3516日夜检测线程已停止");
}

void CDayNightController::setSensitivity(unsigned int sensitivity)
{
    /* 灵敏度由检测线程逐轮原子读取，无需额外修改已接受日夜运行态。 */
    m_sensitivity.store(sensitivity);
}

int CDayNightController::set_tuning_profile(const Hi3516TuningProfile_S *pProfile)
{
    if (pProfile == nullptr)
    {
        return ERR_PARAM_NULL;
    }
    m_pTuningProfile = pProfile;
    return OK;
}

int CDayNightController::sync_runtime_context(const ISP::IspDayNightObservationContext_S &stContext)
{
    {
        /* lock: 运行场景、IR状态和候选在同一锁域更新，禁止检测线程读取混合基线。 */
        std::lock_guard<std::mutex> stLock(m_mtxObservationState);
        m_irAttr.ir_status = stContext.bIsNight ? OT_ISP_IR_STATUS_IR : OT_ISP_IR_STATUS_NORMAL;
        m_isNight.store(stContext.bIsNight);
        m_enRuntimeScene = stContext.enRuntimeScene;
        /* 已接受运行态即共享filter对候选的最终裁决，旧候选不得继续影响后续观测。 */
        m_bObservationPending = false;
        m_bPendingSuggestedNight = false;
    }
    dlog_info("Hi3516日夜检测运行态已同步, state:%s, runtime_scene:%d, requested_light:%d, actual_light:%d",
              stContext.bIsNight ? "夜间" : "白天",
              static_cast<int>(stContext.enRuntimeScene),
              static_cast<int>(stContext.enRequestedLightType),
              static_cast<int>(stContext.enActualLightType));
    return OK;
}

void CDayNightController::setStateChangeCallback(StateChangeCallback callback)
{
    std::lock_guard<std::mutex> stLock(m_mtxStateChangeCallback);
    m_stateChangeCallback = std::move(callback);
}

void CDayNightController::workerThread()
{
    while (m_running.load())
    {
        handleAutoMode();
        /* perf: 检测频率受 200ms 周期限制，防止高频查询 ISP 统计接口。 */
        std::this_thread::sleep_for(THREAD_SLEEP_INTERVAL);
    }
}

void CDayNightController::handleAutoMode()
{
    ObservationSample_S stSample;
    ObservationEvent_S stEvent;
    uint32_t nDayToNightThresh = DAY_TO_NIGHT_THRESH_BY_SENSITIVITY[2];
    {
        /* lock: MPI接口读写m_irAttr期间，候选计算必须基于同一份已接受运行态快照。 */
        std::lock_guard<std::mutex> stObservationLock(m_mtxObservationState);
        if (collect_observation_sample(stSample) != OK)
        {
            return;
        }

        /* 板端创建 ispDayNightDebug 文件时，周期打印当前光照亮度及分量，用于调参和排查。 */
        if (access("/tmp/ispDayNightDebug", F_OK) == 0)
        {
            dlog_info("日夜检测采样, state:%s, scene:%d, sensitivity:%u, "
                      "exposure:%u, ave_lum:%u, bright:%llu, rg:%u, bg:%u",
                      m_isNight.load() ? "夜间" : "白天",
                      static_cast<int>(m_enRuntimeScene),
                      m_sensitivity.load(),
                      static_cast<unsigned int>(stSample.nExposure),
                      static_cast<unsigned int>(stSample.nAveLum),
                      static_cast<unsigned long long>(stSample.u64Bright),
                      static_cast<unsigned int>(stSample.nRg),
                      static_cast<unsigned int>(stSample.nBg));
        }

        int nCurrentSensitivity = static_cast<int>(m_sensitivity.load());
        if (nCurrentSensitivity >= 0 && nCurrentSensitivity < DAY_TO_NIGHT_THRESH_COUNT)
        {
            nDayToNightThresh = DAY_TO_NIGHT_THRESH_BY_SENSITIVITY[nCurrentSensitivity];
        }

        const bool bAcceptedNight = m_isNight.load();
        const bool bConditionMet = bAcceptedNight ? is_night_to_day_condition_met(stSample)
                                                  : is_day_to_night_condition_met(stSample, nDayToNightThresh);
        stEvent = update_observation_candidate_locked(bConditionMet, !bAcceptedNight);
    }

    if (stEvent.enType == ObservationEventType_E::NONE)
    {
        return;
    }

    if (stEvent.enType == ObservationEventType_E::CANDIDATE_CANCELLED)
    {
        dlog_info("Hi3516日夜检测候选已取消, pending_target:%s, current_state:%s, exposure:%u, "
                  "ave_lum:%u, bright:%llu, rg:%u, bg:%u",
                  stEvent.bPendingTargetNight ? "夜间" : "白天",
                  stEvent.bSuggestedNight ? "夜间" : "白天",
                  static_cast<unsigned int>(stSample.nExposure),
                  static_cast<unsigned int>(stSample.nAveLum),
                  static_cast<unsigned long long>(stSample.u64Bright),
                  static_cast<unsigned int>(stSample.nRg),
                  static_cast<unsigned int>(stSample.nBg));
    }
    else
    {
        dlog_info("Hi3516日夜检测产生建议, target:%s, exposure:%u, ave_lum:%u, bright:%llu, "
                  "day_to_night_thresh:%u, rg:%u, bg:%u",
                  stEvent.bSuggestedNight ? "夜间" : "白天",
                  static_cast<unsigned int>(stSample.nExposure),
                  static_cast<unsigned int>(stSample.nAveLum),
                  static_cast<unsigned long long>(stSample.u64Bright),
                  nDayToNightThresh,
                  static_cast<unsigned int>(stSample.nRg),
                  static_cast<unsigned int>(stSample.nBg));
    }

    report_observation(stEvent.bSuggestedNight);
}

int CDayNightController::collect_observation_sample(ObservationSample_S &stSample)
{
    /* memory: MPI接口只操作局部快照，禁止其把未通过共享filter的建议写回已接受运行态基线。 */
    ot_isp_ir_auto_attr stIrAttr = m_irAttr;
    int nRet = ss_mpi_isp_ir_auto(m_viPipe, &stIrAttr);
    if (nRet != OK)
    {
        report_collect_error(CollectStage_E::IR_AUTO, nRet);
        return ERR;
    }

    ot_isp_exp_info stExposureInfo{};
    nRet = ss_mpi_isp_query_exposure_info(m_viPipe, &stExposureInfo);
    if (nRet != OK)
    {
        report_collect_error(CollectStage_E::EXPOSURE_INFO, nRet);
        return ERR;
    }

    /* 白平衡统计用于区分补光造成的亮度变化和真实环境变亮。 */
    ot_isp_wb_stats stWbStats{};
    nRet = ss_mpi_isp_get_wb_stats(m_viPipe, &stWbStats);
    if (nRet != OK)
    {
        report_collect_error(CollectStage_E::WB_STATS, nRet);
        return ERR;
    }
    clear_collect_error();

    stSample.nExposure = stExposureInfo.exposure;
    stSample.nAveLum = stExposureInfo.ave_lum;
    /* 以G通道归一化R/B比值；div_0_to_1防止弱光场景G为零时除零。 */
    stSample.nRg = (static_cast<td_u32>(stWbStats.global_r) << SHIFT_8BIT) / div_0_to_1(stWbStats.global_g);
    stSample.nBg = (static_cast<td_u32>(stWbStats.global_b) << SHIFT_8BIT) / div_0_to_1(stWbStats.global_g);
    stSample.u64Bright = static_cast<uint64_t>(stSample.nExposure) * stSample.nAveLum;
    return OK;
}

bool CDayNightController::is_day_to_night_condition_met(const ObservationSample_S &stSample, uint32_t nDayToNightThresh) const
{
    const bool bMetStandard = stSample.u64Bright > nDayToNightThresh;
    const bool bMetPitchBlack = stSample.nAveLum < 5 && stSample.nExposure > 200000;
    return bMetStandard || bMetPitchBlack;
}

bool CDayNightController::is_night_to_day_condition_met(const ObservationSample_S &stSample) const
{
    /* 默认阈值为未注入画像时的安全回退，机型画像存在时整体替换。 */
    uint64_t u64WhiteBright = DEFAULT_WHITE_TO_DAY_BRIGHT;
    uint64_t u64RedBright = DEFAULT_RED_TO_DAY_BRIGHT;
    td_u32 nWhiteRg = DEFAULT_WHITE_RG;
    td_u32 nWhiteBg = DEFAULT_WHITE_BG;
    td_u32 nRedRg = DEFAULT_RED_RG;
    td_u32 nRedBg = DEFAULT_RED_BG;
    if (m_pTuningProfile != nullptr)
    {
        const DayNightThreshProfile_S &stThresh = m_pTuningProfile->stDayNightThresh;
        u64WhiteBright = stThresh.u64WhiteLightToDayBright;
        u64RedBright = stThresh.u64RedLightToDayBright;
        nWhiteRg = stThresh.nWhiteRg;
        nWhiteBg = stThresh.nWhiteBg;
        nRedRg = stThresh.nRedRg;
        nRedBg = stThresh.nRedBg;
    }

    // review: 外设总控禁灯或场景关闭补光时，当前尚无独立的无灯夜转日标定值。
    // 暂按运行场景复用现有白光/红外阈值，保证自动模式仍可返回白天。
    // 后续完成不同镜头或机型的无灯实机标定后，再由能力画像或调参画像提供独立阈值。
    if (m_enRuntimeScene == ISP::IspRuntimeScene_E::NIGHT_WHITE)
    {
        return stSample.u64Bright < u64WhiteBright && stSample.nRg < nWhiteRg && stSample.nBg < nWhiteBg;
    }
    if (m_enRuntimeScene == ISP::IspRuntimeScene_E::NIGHT_IR || m_enRuntimeScene == ISP::IspRuntimeScene_E::NIGHT_SMART ||
        m_enRuntimeScene == ISP::IspRuntimeScene_E::NIGHT_LIGHT_OFF)
    {
        return stSample.u64Bright < u64RedBright && stSample.nRg < nRedRg && stSample.nBg < nRedBg;
    }
    return false;
}

CDayNightController::ObservationEvent_S CDayNightController::update_observation_candidate_locked(bool bConditionMet,
                                                                                                 bool bTargetNight)
{
    ObservationEvent_S stEvent;
    const bool bAcceptedNight = m_isNight.load();
    if (!m_bObservationPending)
    {
        if (!bConditionMet)
        {
            return stEvent;
        }

        m_bObservationPending = true;
        m_bPendingSuggestedNight = bTargetNight;
        stEvent.enType = ObservationEventType_E::CANDIDATE_STARTED;
        stEvent.bSuggestedNight = bTargetNight;
        return stEvent;
    }

    if (bConditionMet && m_bPendingSuggestedNight == bTargetNight)
    {
        /* 连续满足同一候选时保持静默，禁止重置共享层首次建议建立的filter deadline。 */
        return stEvent;
    }

    if (!bConditionMet)
    {
        stEvent.enType = ObservationEventType_E::CANDIDATE_CANCELLED;
        stEvent.bSuggestedNight = bAcceptedNight;
        stEvent.bPendingTargetNight = m_bPendingSuggestedNight;
        m_bObservationPending = false;
        m_bPendingSuggestedNight = false;
        return stEvent;
    }

    /* review: 已接受状态未变化时目标理论上不会反转；仍更新候选以防平台扩展出双向原始判定。 */
    m_bPendingSuggestedNight = bTargetNight;
    stEvent.enType = ObservationEventType_E::CANDIDATE_STARTED;
    stEvent.bSuggestedNight = bTargetNight;
    return stEvent;
}

void CDayNightController::report_collect_error(CollectStage_E enStage, int nRet)
{
    if (m_enLastCollectErrorStage == enStage && m_nLastCollectErrorCode == nRet)
    {
        return;
    }

    m_enLastCollectErrorStage = enStage;
    m_nLastCollectErrorCode = nRet;
    dlog_error("Hi3516日夜检测数据采集失败, stage:%s, vi_pipe:%d, ret:%d", collect_stage_name(enStage), m_viPipe, nRet);
}

void CDayNightController::clear_collect_error()
{
    if (m_enLastCollectErrorStage == CollectStage_E::NONE)
    {
        return;
    }

    dlog_info("Hi3516日夜检测数据采集已恢复, previous_stage:%s, previous_ret:%d",
              collect_stage_name(m_enLastCollectErrorStage),
              m_nLastCollectErrorCode);
    m_enLastCollectErrorStage = CollectStage_E::NONE;
    m_nLastCollectErrorCode = OK;
}

const char *CDayNightController::collect_stage_name(CollectStage_E enStage)
{
    switch (enStage)
    {
    case CollectStage_E::IR_AUTO:
        return "IR_AUTO";
    case CollectStage_E::EXPOSURE_INFO:
        return "EXPOSURE_INFO";
    case CollectStage_E::WB_STATS:
        return "WB_STATS";
    case CollectStage_E::NONE:
    default:
        return "NONE";
    }
}

void CDayNightController::report_observation(bool bSuggestedNight)
{
    /* memory: 复制std::function后立即释放回调锁，调用链可以安全进入共享mode controller。 */
    StateChangeCallback stCallback;
    {
        std::lock_guard<std::mutex> stLock(m_mtxStateChangeCallback);
        stCallback = m_stateChangeCallback;
    }

    if (stCallback)
    {
        stCallback(bSuggestedNight, ISP::AUTO_MODE);
    }
}
