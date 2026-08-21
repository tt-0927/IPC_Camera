/**
 * @FilePath     : isp_dayNight.h
 * @Author       : cyc
 * @Date         : 2026-02-27 13:40:43
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-27 15:35:33
 * @Description  : 日夜自动检测控制器（仅自动观测，mode/time/filter由共享mode controller负责）
 */

#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include "Singleton.h"
#include "isp_tuning_profile.h"
#include "isp_define.h"
#include "isp_runtime_decision.h"
#include "ot_mpi_isp.h"

class CDayNightController : public CSingleton<CDayNightController>
{
public:
    /**
     * @brief   : 自动日夜状态改变回调，仅传递检测建议。
     * @return   {void}
     */
    using StateChangeCallback = std::function<void(bool isNight, ISP::DayNightMode_E mode)>;

    /**
     * @brief   : 构造底层自动日夜检测控制器并填充MPP默认阈值。
     * @return   {void}
     */
    CDayNightController();
    /**
     * @brief   : 停止检测线程。
     * @return   {void}
     */
    ~CDayNightController();
    friend class CSingleton<CDayNightController>;

    /**
     * @brief   : 启动自动环境光检测线程。
     * @return   {bool} true：已启动或已运行，false：创建失败
     * @note    : 保留sync_runtime_context写入的已接受状态和连续检测候选，不在启动时重置为白天。
     */
    bool start();
    /**
     * @brief   : 请求检测线程退出并等待其完成。
     * @return   {void}
     */
    void stop();

    /**
     * @brief   : 更新自动日夜灵敏度并在下次轮询刷新阈值。
     * @param    {unsigned int} sensitivity：灵敏度等级
     * @return   {void}
     */
    void setSensitivity(unsigned int sensitivity);
    /**
     * @brief   : 注入机型相关夜转日阈值画像。
     * @param    {const Hi3516TuningProfile_S*} pProfile：非拥有画像指针
     * @return   {int} OK：成功，ERR_PARAM_NULL：空指针
     */
    int set_tuning_profile(const Hi3516TuningProfile_S *pProfile);
    /**
     * @brief   : 同步共享层已接受的运行态并清除已消费的检测候选。
     * @param    {const ISP::IspDayNightObservationContext_S&} stContext：最后成功运行态上下文
     * @return   {int} OK：同步成功
     * @note    : 仅更新观测基线，不直接操作场景、IR-CUT或补光硬件。
     */
    int sync_runtime_context(const ISP::IspDayNightObservationContext_S &stContext);

    /**
     * @brief   : 查询共享层已接受的夜间运行态。
     * @return   {bool} true：夜间，false：白天
     */
    bool isNightMode() const
    {
        return m_isNight.load();
    }
    /**
     * @brief   : 设置状态变化回调。
     * @param    {StateChangeCallback} callback：共享层观测回调
     * @return   {void}
     */
    void setStateChangeCallback(StateChangeCallback callback);

private:
    /**
     * @brief 自动日夜观测采样快照。
     */
    struct ObservationSample_S
    {
        td_u32 nExposure;
        td_u32 nAveLum;
        td_u32 nRg;
        td_u32 nBg;
        uint64_t u64Bright;

        ObservationSample_S() : nExposure(0), nAveLum(0), nRg(0), nBg(0), u64Bright(0)
        {
        }
    };

    /**
     * @brief detector候选事件类型。
     */
    enum class ObservationEventType_E
    {
        NONE,
        CANDIDATE_STARTED,
        CANDIDATE_CANCELLED,
    };

    /**
     * @brief detector候选事件。
     */
    struct ObservationEvent_S
    {
        ObservationEventType_E enType;
        bool bSuggestedNight;
        bool bPendingTargetNight;

        ObservationEvent_S() : enType(ObservationEventType_E::NONE), bSuggestedNight(false), bPendingTargetNight(false)
        {
        }
    };

    /**
     * @brief ISP日夜检测数据采集阶段。
     */
    enum class CollectStage_E
    {
        NONE,
        IR_AUTO,
        EXPOSURE_INFO,
        WB_STATS,
    };

    /**
     * @brief   : 检测线程主循环。
     * @return   {void}
     */
    void workerThread();
    /**
     * @brief   : 读取 ISP 统计值并计算自动日夜建议。
     * @return   {void}
     */
    void handleAutoMode();
    /**
     * @brief   : 采集一组ISP曝光与白平衡观测值。
     * @param    {ObservationSample_S&} stSample：输出的采样快照
     * @return   {int} OK：采集成功，非OK：对应ISP接口失败
     * @note    : 调用方必须持有m_mtxObservationState，保证MPP IR快照不被运行态同步并发修改。
     */
    int collect_observation_sample(ObservationSample_S &stSample);
    /**
     * @brief   : 判断已接受白天状态是否满足切换夜间的原始条件。
     * @param    {const ObservationSample_S&} stSample：当前采样快照
     * @param    {uint32_t} nDayToNightThresh：当前灵敏度对应阈值
     * @return   {bool} true：满足夜间候选条件，false：不满足
     */
    bool is_day_to_night_condition_met(const ObservationSample_S &stSample, uint32_t nDayToNightThresh) const;
    /**
     * @brief   : 判断已接受夜间状态是否满足切换白天的原始条件。
     * @param    {const ObservationSample_S&} stSample：当前采样快照
     * @return   {bool} true：满足白天候选条件，false：不满足
     */
    bool is_night_to_day_condition_met(const ObservationSample_S &stSample) const;
    /**
     * @brief   : 根据原始条件维护候选状态并生成启动或撤销事件。
     * @param    {bool} bConditionMet：当前采样是否满足反向切换条件
     * @param    {bool} bTargetNight：反向候选是否为夜间
     * @return   {ObservationEvent_S} 本轮需要上报的候选事件；NONE表示无需上报
     * @note    : 调用方必须持有m_mtxObservationState。
     */
    ObservationEvent_S update_observation_candidate_locked(bool bConditionMet, bool bTargetNight);
    /**
     * @brief   : 在不持有观测状态锁时调用订阅回调。
     * @param    {bool} bSuggestedNight：建议夜间状态
     * @return   {void}
     */
    void report_observation(bool bSuggestedNight);
    /**
     * @brief   : 仅在采集错误首次出现或状态变化时记录日志。
     * @param    {CollectStage_E} enStage：失败的采集阶段
     * @param    {int} nRet：MPI接口返回码
     * @return   {void}
     */
    void report_collect_error(CollectStage_E enStage, int nRet);
    /**
     * @brief   : 数据采集恢复时记录一次恢复日志并清除错误状态。
     * @return   {void}
     */
    void clear_collect_error();
    /**
     * @brief   : 获取数据采集阶段名称。
     * @param    {CollectStage_E} enStage：数据采集阶段
     * @return   {const char*} 静态阶段名称
     */
    static const char *collect_stage_name(CollectStage_E enStage);

    /* memory: 线程对象由控制器独占，stop/destructor 负责 join。 */
    std::unique_ptr<std::thread> m_workerThread;
    std::atomic<bool> m_running{ false };

    /* lock: 保护MPP IR快照、已接受运行态和检测候选，避免共享同步线程与检测线程并发读写。 */
    mutable std::mutex m_mtxObservationState;
    /* 已接受运行态由sync_runtime_context独占写入；外部查询使用原子读取。 */
    std::atomic<bool> m_isNight{ false };
    bool m_bObservationPending{ false };
    bool m_bPendingSuggestedNight{ false };

    std::atomic<unsigned int> m_sensitivity{ 1 };
    /* 最后成功运行场景与候选状态使用m_mtxObservationState统一保护。 */
    ISP::IspRuntimeScene_E m_enRuntimeScene{ ISP::IspRuntimeScene_E::DAY };
    /* memory: 画像归启动器所有，必须在本控制器停止后才可销毁。 */
    const Hi3516TuningProfile_S *m_pTuningProfile{ nullptr };

    ot_isp_ir_auto_attr m_irAttr;
    int m_viPipe{ 0 };

    /* 启动前由生命周期线程初始化，运行期间仅由检测线程读写，用于去重连续采集错误。 */
    CollectStage_E m_enLastCollectErrorStage{ CollectStage_E::NONE };
    int m_nLastCollectErrorCode;

    /* 回调由适配器注册；状态变化时复制后在锁外调用以避免锁重入。 */
    StateChangeCallback m_stateChangeCallback;
    mutable std::mutex m_mtxStateChangeCallback;

    /* perf: 环境光变化速度远低于视频帧率，200ms周期可降低ISP锁竞争和线程唤醒。 */
    static constexpr auto THREAD_SLEEP_INTERVAL = std::chrono::milliseconds(200);
    static constexpr unsigned int NORMAL_TO_IR_THRESHOLD = 24000;
    static constexpr unsigned int IR_TO_NORMAL_THRESHOLD = 500;
};
