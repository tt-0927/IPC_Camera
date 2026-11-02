/**
 * @FilePath     : rv1126b_ipc/main_app/ai_app/detect_mode/pmnm_algo_detect/pmnm_algo_detect.hpp
 * @Date         : 2026-10-12
 * @Description  : 人车非检测业务算法（继承 CAlgorithm 取流框架，推理内核复用 algorithm_mode 纯检测组件）
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "algorithm.hpp"
#include "blocking_queue.hpp"
#include "queue_crowding_processor.hpp"
#include "region_stay_processor.hpp"
#include "tracker.hpp"

/* 人车非跟踪类别数 */
#define PMNM_TRACK_CLASS_COUNT (3)

/* 纯检测组件（algorithm_mode/pmnm_detect），仅持有指针，前置声明 */
class CPMNMDetect;

/* 后厨穿戴规范子检测器（sub_detectors/），仅持有指针，前置声明 */
class CKitchenClothingSubDetector;

/**
 * @brief  : 人车非检测业务算法（CAlgorithm 子类）
 * @note   : 取流框架对照 detect_mode 既有算法（线程 + 阻塞队列 + 500ms 节流 + lazy init），
 *           推理内核调用 algorithm_mode 纯检测组件 CPMNMDetect::process()。
 *           配置接入：攀爬识别/危险区域靠近/长时间滞留/危险区域滞留/重点区域异常滞留/天台滞留
 *           （setAlgoEnCfg 总开关 + setAlgoParamCfg 详细规则，
 *           区域/灵敏度/停留时长经 region_stay_processor 判定）。
 *           跟踪接入/判定/触发消费已接；报警状态机（algorithm.hpp CAlarmStateMachine）
 *           驱动事件开始/结束走平台联动上报（CEventLinkage）。
 */
class CPMNMAlgoDetect : public CAlgorithm
{
public:
    CPMNMAlgoDetect();
    ~CPMNMAlgoDetect();

    /**
     * @brief   : 接受媒体数据（CAlgorithm 取流入口）
     * @param    {MediaData_S} stMediaData：媒体数据（NV12）
     * @note    : 内部 500ms 节流后入队，由 run 线程异步消费
     */
    void recvMediaData(MediaData_S stMediaData) override;

    /**
     * @brief   : 更新算法配置参数（CAlgorithm 配置入口）
     * @param    {Event::AlgorithmConfig} &stAlgoConfig：算法开关配置
     * @note    : 任一事件启用即拉起取流；启用的事件拉取详细配置下发处理器
     */
    void setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig) override;

    /**
     * @brief   : 更新攀爬识别检测参数
     * @param    {Alarm::ClimbDetection_S} &stAlgoCfg：攀爬识别检测配置
     * @note    : 区域由 1920x1080 基准转到 AI 检测帧分辨率后直接配置处理器
     */
    void setAlgoParamCfg(const Alarm::ClimbDetection_S &stAlgoCfg);

    /**
     * @brief   : 更新危险区域靠近检测参数
     * @param    {Alarm::DangerZoneApproach_S} &stAlgoCfg：危险区域靠近检测配置
     * @note    : 区域由 1920x1080 基准转到 AI 检测帧分辨率后直接配置处理器
     */
    void setAlgoParamCfg(const Alarm::DangerZoneApproach_S &stAlgoCfg);

    /**
     * @brief   : 更新长时间滞留参数
     * @param    {Alarm::LongTimeStayDetection_S} &stAlgoCfg：长时间滞留配置
     * @note    : 区域由 1920x1080 基准转到 AI 检测帧分辨率后直接配置处理器
     */
    void setAlgoParamCfg(const Alarm::LongTimeStayDetection_S &stAlgoCfg);

    /**
     * @brief   : 更新危险区域滞留参数
     * @param    {Alarm::DangerZoneStayDetection_S} &stAlgoCfg：危险区域滞留配置
     * @note    : 区域由 1920x1080 基准转到 AI 检测帧分辨率后直接配置处理器
     */
    void setAlgoParamCfg(const Alarm::DangerZoneStayDetection_S &stAlgoCfg);

    /**
     * @brief   : 更新重点区域异常滞留参数
     * @param    {Alarm::KeyAreaAbnormalStayDetection_S} &stAlgoCfg：重点区域异常滞留配置
     * @note    : 区域由 1920x1080 基准转到 AI 检测帧分辨率后直接配置处理器
     */
    void setAlgoParamCfg(const Alarm::KeyAreaAbnormalStayDetection_S &stAlgoCfg);

    /**
     * @brief   : 更新天台滞留参数
     * @param    {Alarm::RooftopStayDetection_S} &stAlgoCfg：天台滞留配置
     * @note    : 区域由 1920x1080 基准转到 AI 检测帧分辨率后直接配置处理器
     */
    void setAlgoParamCfg(const Alarm::RooftopStayDetection_S &stAlgoCfg);

    /**
     * @brief   : 更新人员徘徊检测参数
     * @param    {Alarm::PersonLoitering_S} &stAlgoCfg：人员徘徊检测配置
     * @note    : 区域由 1920x1080 基准转到 AI 检测帧分辨率后直接配置处理器
     */
    void setAlgoParamCfg(const Alarm::LoiteringDetection_S &stAlgoCfg);

    /**
     * @brief   : 更新排队拥堵检测参数
     * @param    {Alarm::QueueCrowdingDetection_S} &stAlgoCfg：排队拥堵检测配置
     * @note    : 区域由 1920x1080 基准转到 AI 检测帧分辨率后直接配置处理器
     */
    void setAlgoParamCfg(const Alarm::QueueCrowdingDetection_S &stAlgoCfg);

private:
    /**
     * @brief   : 初始化（创建并加载纯检测组件，run 线程内 lazy 调用，失败可重试）
     * @return   {bool} true：成功 false：失败
     */
    bool init();

    /**
     * @brief   : 反初始化（释放纯检测组件）
     * @return   {bool} true：成功
     * @note    : 重复调用安全
     */
    bool unInit();

    /**
     * @brief   : 数据处理线程（取流 → NV12 转 RGB → 检测 → 跟踪 → 判定）
     */
    void run();

    /**
     * @brief   : 处理区域停留类事件（攀爬/靠近/滞留/徘徊等，通用逻辑）
     * @param    {bool} bEnable：当前事件是否启用
     * @param    {Event::Type} enType：事件类型
     * @param    {CRegionStayProcessor &} processor：对应的判定处理器
     * @param    {CAlarmStateMachine &} stateMachine：对应的报警状态机
     * @param    {int64_t} nFrameTsMs：帧时间戳
     * @param    {std::vector<TrackedTarget_S> &} vecTrackedTarget：跟踪目标列表
     */
    void processRegionStayEvent(bool bEnable, Event::Type enType,
                                CRegionStayProcessor &processor,
                                CAlarmStateMachine &stateMachine,
                                const int64_t nFrameTsMs,
                                const std::vector<TrackedTarget_S> &vecTrackedTarget);

    /**
     * @brief   : 处理排队拥堵事件（触发类型与区域停留不同，单独处理）
     */
    void processQueueCrowding(const int64_t nFrameTsMs, const std::vector<TrackedTarget_S> &vecTrackedTarget);

private:
    /* 纯检测组件实例（推理内核） */
    CPMNMDetect *m_pDetectComponent = nullptr;

    /* 后厨穿戴规范子检测器（发布订阅：开启时创建并通知其自拉配置，依赖人员目标输出） */
    std::unique_ptr<CKitchenClothingSubDetector> m_pKitchenClothingSubDetector;

    /* 数据队列 */
    BQ_NS::CBlockingQueue<MediaData_S> m_dateQueue;

    /* 用于控制线程的运行 */
    std::atomic<bool>       m_bRunning;
    /* 互斥锁保护指针访问 */
    std::mutex              m_mutex;
    /* 条件变量 */
    std::condition_variable m_condition;
    /* 数据处理线程 */
    std::thread             m_thread;

    /* 检测频率控制（500ms 节流） */
    EventManager m_RecvManager{500};

    /* 跟踪封装（内部 per-class 实例：外层下标即类别，与 PMNM_CLASS_XXX 对应） */
    Tracker m_oTracker;

    /* 事件处理相关 */
    Alarm::DangerZoneApproach_S m_stDangerZoneCfg;
    Alarm::ClimbDetection_S m_stClimbCfg;
    Alarm::LongTimeStayDetection_S m_stLongTimeStayCfg;
    Alarm::DangerZoneStayDetection_S m_stDangerZoneStayCfg;
    Alarm::KeyAreaAbnormalStayDetection_S m_stKeyAreaAbnormalStayCfg;
    Alarm::RooftopStayDetection_S m_stRooftopStayCfg;
    Alarm::LoiteringDetection_S m_stPersonLoiteringCfg;
    Alarm::QueueCrowdingDetection_S m_stQueueCrowdingCfg;

    /* 后厨穿戴规范事件开关（发布订阅：开启即创建子检测器，配置由其自行拉取） */
    std::atomic<bool> m_bKitchenClothingEnable{false};

    /* 判定处理器：攀爬识别/危险区域靠近/长时间滞留/危险区域滞留/重点区域异常滞留/天台滞留
       （均仅人目标，攀爬区域内每帧触发，其余停留达阈值周期触发） */
    CRegionStayProcessor m_climbProcessor{Event::Type::CLIMB_DETECTION};
    CRegionStayProcessor m_dangerZoneProcessor{Event::Type::DANGER_ZONE_APPROACH};
    CRegionStayProcessor m_longTimeStayProcessor{Event::Type::LONG_TIME_STAY};
    CRegionStayProcessor m_dangerZoneStayProcessor{Event::Type::DANGER_ZONE_STAY};
    CRegionStayProcessor m_keyAreaAbnormalStayProcessor{Event::Type::KEY_AREA_ABNORMAL_STAY};
    CRegionStayProcessor m_rooftopStayProcessor{Event::Type::ROOFTOP_STAY};
    CRegionStayProcessor m_personLoiteringProcessor{Event::Type::LOITERING_DETECT};
    CQueueCrowdingProcessor m_queueCrowdingProcessor{Event::Type::QUEUE_CROWDING};

    /* 报警状态机：冷却与重报节奏内置（algorithm.hpp），开始/结束自动走 CEventLinkage */
    CAlarmStateMachine m_climbAlarmStateMachine;
    CAlarmStateMachine m_dangerZoneAlarmStateMachine;
    CAlarmStateMachine m_longTimeStayAlarmStateMachine;
    CAlarmStateMachine m_dangerZoneStayAlarmStateMachine;
    CAlarmStateMachine m_keyAreaAbnormalStayAlarmStateMachine;
    CAlarmStateMachine m_rooftopStayAlarmStateMachine;
    CAlarmStateMachine m_personLoiteringAlarmStateMachine;
    CAlarmStateMachine m_queueCrowdingAlarmStateMachine;

    /* 算法开关：任一事件启用（setAlgoEnCfg 驱动） */
    std::atomic<bool> m_bAlgoEnable{false};
};
