/**
 * @FilePath     : rv1126b_ipc/main_app/ai_app/detect_mode/pmnm_algo_detect/pmnm_algo_detect.cpp
 * @Date         : 2026-10-12
 * @Description  : 人车非检测业务算法实现（取流框架，推理内核复用 algorithm_mode 纯检测组件）
 */

#include "pmnm_algo_detect.hpp"

#include <algorithm>
#include <chrono>
#include <vector>

#include "algorithm_define.hpp"
#include "common_process.h"
#include "kitchen_clothing_sub_detector.hpp"
#include "group4_sub_detector.hpp"
#include "crowd_gathering_sub_detector.hpp"
#include "pmnm_detect.hpp"
#include "video_define.h"
#include "share_data.h"

/* 数据队列容量 */
#define QUEUE_MAX (2)

/* 最大丢失帧数：节流后 ≈2fps，4 帧 ≈ 2s */
#define TRACKER_MAX_TIME_LOST (4)

/* 跟踪参数（阈值取 Group2 实测，落码后按实拍调参） */
static Tracker::TrackerConfig makeTrackerConfig()
{
    Tracker::TrackerConfig stConfig;
    stConfig.nClassCount    = PMNM_TRACK_CLASS_COUNT;
    stConfig.vecTrackThresh = {0.5f, 0.6f, 0.5f};
    stConfig.vecHighThresh  = {0.6f, 0.5f, 0.4f};
    stConfig.vecMatchThresh = {0.5f, 0.7f, 0.8f};
    stConfig.vecFrameId     = {0, 0, 0};
    stConfig.nMaxTimeLost   = TRACKER_MAX_TIME_LOST;
    return stConfig;
}

/* 配置区域为 1920x1080 基准，转换到 AI 检测帧分辨率（VPSS_CHANNEL_AI 同源宏）；区域无效返回 false */
static bool convertRegionToAiFrame(const Alarm::Region_S &stSrcRegion, Alarm::Region_S &stDstRegion)
{
    stDstRegion = stSrcRegion;
    if (!stDstRegion.IsValid())
    {
        return false;
    }
    stDstRegion.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, PIXEL_WIDTH_AI, PIXEL_HEIGHT_AI);
    return true;
}

/* 事件配置规则 → 处理器判定规则（未绘制区域不参与判定） */
static bool makeRegionRule(const Alarm::Region_S &stRegion, unsigned int nSensitivity,
                           unsigned int nTimeThresholdSec, RegionRule_S &stRule)
{
    if (!convertRegionToAiFrame(stRegion, stRule.stRegion))
    {
        return false;
    }

    stRule.nRuleId = -1;
    stRule.nTimeThresholdSec = nTimeThresholdSec;
    stRule.nSensitivity = nSensitivity;
    return true;
}

CPMNMAlgoDetect::CPMNMAlgoDetect()
 : m_dateQueue(QUEUE_MAX)
{
    m_bRunning.store(true);
    m_thread = std::thread(&CPMNMAlgoDetect::run, this);
}

CPMNMAlgoDetect::~CPMNMAlgoDetect()
{
    /* 通知线程停止 */
    m_bRunning.store(false);
    m_condition.notify_all();
    MediaData_S stMediaData;
    m_dateQueue.pushOrReplace(stMediaData);
    if (m_thread.joinable())
    {
        m_thread.join();
    }
    unInit();
}

void CPMNMAlgoDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_bAlgoEnable.load())
    {
        dlog_debug("PMNM_ALGO_DETECT: 算法开关未启用");
        return;
    }

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("PMNM_ALGO_DETECT: 数据队列满了");
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

void CPMNMAlgoDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stClimbCfg.bEnable        = stAlgoConfig.nClimbDetection;
    m_stDangerZoneCfg.bEnable   = stAlgoConfig.nDangerZoneApproach;
    m_stLongTimeStayCfg.bEnable = stAlgoConfig.nLongTimeStay;
    m_stDangerZoneStayCfg.bEnable = stAlgoConfig.nDangerZoneStay;
    m_stKeyAreaAbnormalStayCfg.bEnable = stAlgoConfig.nKeyAreaAbnormalStay;
    m_stRooftopStayCfg.bEnable  = stAlgoConfig.nRooftopStay;
    m_stPersonLoiteringCfg.bEnable = stAlgoConfig.nEnLoiteringDetect;
    m_stQueueCrowdingCfg.bEnable = stAlgoConfig.nEnQueueCrowding;
    m_bKitchenClothingEnable = (stAlgoConfig.nKitchenClothingCompliance != 0);
    m_bCrowdGatheringEnable = (stAlgoConfig.nEnCrowdGathering != 0);

    /* 启用的事件拉取详细配置并直接下发处理器 */
    if (stAlgoConfig.nClimbDetection)
    {
        Alarm::ClimbDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (stAlgoConfig.nDangerZoneApproach)
    {
        Alarm::DangerZoneApproach_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (stAlgoConfig.nLongTimeStay)
    {
        Alarm::LongTimeStayDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (stAlgoConfig.nDangerZoneStay)
    {
        Alarm::DangerZoneStayDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (stAlgoConfig.nKeyAreaAbnormalStay)
    {
        Alarm::KeyAreaAbnormalStayDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (stAlgoConfig.nRooftopStay)
    {
        Alarm::RooftopStayDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (stAlgoConfig.nEnLoiteringDetect)
    {
        Alarm::LoiteringDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (stAlgoConfig.nEnQueueCrowding)
    {
        Alarm::QueueCrowdingDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    /* 后厨穿戴规范（发布订阅）：开启即创建子检测器并通知其自拉配置，规则下发不下沉到本层 */
    if (m_bKitchenClothingEnable)
    {
        if (!m_pKitchenClothingSubDetector)
        {
            m_pKitchenClothingSubDetector = std::make_unique<CKitchenClothingSubDetector>();
        }
        m_pKitchenClothingSubDetector->updateConfig();
    }

    /* Group4 行为检测（抽烟/睡岗/玩手机/摔倒）：开启即创建子检测器并通知其自拉配置 */
    m_bGroup4Enable = (stAlgoConfig.nEnSmoking != 0 || stAlgoConfig.nEnSleepOnDuty != 0 ||
                       stAlgoConfig.nEnPhoneUsage != 0 || stAlgoConfig.nEnTrip != 0);
    if (m_bGroup4Enable)
    {
        if (!m_pGroup4SubDetector)
        {
            m_pGroup4SubDetector = std::make_unique<CGroup4SubDetector>();
            dlog_info("PMNM_ALGO_DETECT: 创建 Group4 子检测器");
        }
        m_pGroup4SubDetector->updateConfig();
    }

    /* 人员聚集检测：开启即创建子检测器并通知其自拉配置（使用 Group2Detect_NS 模型） */
    if (m_bCrowdGatheringEnable)
    {
        if (!m_pCrowdGatheringSubDetector)
        {
            m_pCrowdGatheringSubDetector = std::make_unique<CCrowdGatheringSubDetector>();
            dlog_info("PMNM_ALGO_DETECT: 创建人员聚集子检测器");
        }
        m_pCrowdGatheringSubDetector->updateConfig();
    }

    /* 任一事件启用即拉起算法取流 */
    m_bAlgoEnable.store(stAlgoConfig.nClimbDetection != 0 || stAlgoConfig.nDangerZoneApproach != 0 ||
                        stAlgoConfig.nLongTimeStay != 0 || stAlgoConfig.nDangerZoneStay != 0 ||
                        stAlgoConfig.nKeyAreaAbnormalStay != 0 || stAlgoConfig.nRooftopStay != 0 ||
                        stAlgoConfig.nEnLoiteringDetect != 0 || stAlgoConfig.nEnQueueCrowding != 0 ||
                        stAlgoConfig.nKitchenClothingCompliance != 0 ||
                        stAlgoConfig.nEnSmoking != 0 || stAlgoConfig.nEnSleepOnDuty != 0 ||
                        stAlgoConfig.nEnPhoneUsage != 0 || stAlgoConfig.nEnTrip != 0 ||
                        stAlgoConfig.nEnCrowdGathering != 0);
    dlog_info("PMNM_ALGO_DETECT: 配置更新 攀爬识别[%d] 危险区域靠近[%d] 长时间滞留[%d] 危险区域滞留[%d] 重点区域异常滞留[%d] 天台滞留[%d] 人员徘徊[%d] 排队拥堵[%d] 后厨穿戴规范[%d] 抽烟[%d] 睡岗[%d] 玩手机[%d] 摔倒[%d] 人员聚集[%d]",
              stAlgoConfig.nClimbDetection, stAlgoConfig.nDangerZoneApproach,
              stAlgoConfig.nLongTimeStay, stAlgoConfig.nDangerZoneStay,
              stAlgoConfig.nKeyAreaAbnormalStay, stAlgoConfig.nRooftopStay,
              stAlgoConfig.nEnLoiteringDetect, stAlgoConfig.nEnQueueCrowding, stAlgoConfig.nKitchenClothingCompliance,
              stAlgoConfig.nEnSmoking, stAlgoConfig.nEnSleepOnDuty, stAlgoConfig.nEnPhoneUsage, stAlgoConfig.nEnTrip,
              stAlgoConfig.nEnCrowdGathering);
}

void CPMNMAlgoDetect::setAlgoParamCfg(const Alarm::ClimbDetection_S &stAlgoCfg)
{
    dlog_debug("PMNM_ALGO_DETECT: 设置攀爬识别检测参数 规则数[%zu]", stAlgoCfg.aRule.size());

    m_stClimbCfg = stAlgoCfg;

    /* 攀爬识别：仅人目标，区域内持续触发（停留阈值 0） */
    std::vector<RegionRule_S> vecRules;
    for (size_t i = 0; i < stAlgoCfg.aRule.size(); ++i)
    {
        RegionRule_S stRule;
        if (!makeRegionRule(stAlgoCfg.aRule[i].stRegion, stAlgoCfg.aRule[i].nSensitivity,
                            0, stRule))
        {
            continue;
        }
        vecRules.push_back(stRule);
    }

    m_climbProcessor.setRules(vecRules);
}

void CPMNMAlgoDetect::setAlgoParamCfg(const Alarm::DangerZoneApproach_S &stAlgoCfg)
{
    dlog_debug("PMNM_ALGO_DETECT: 设置危险区域靠近检测参数 规则数[%zu]", stAlgoCfg.aRule.size());

    m_stDangerZoneCfg = stAlgoCfg;

    /* 危险区域靠近：仅人目标，停留达阈值[0,10]秒报警 */
    std::vector<RegionRule_S> vecRules;
    for (size_t i = 0; i < stAlgoCfg.aRule.size(); ++i)
    {
        RegionRule_S stRule;
        if (!makeRegionRule(stAlgoCfg.aRule[i].stRegion, stAlgoCfg.aRule[i].nSensitivity,
                            stAlgoCfg.aRule[i].nTimeThreshold, stRule))
        {
            continue;
        }
        vecRules.push_back(stRule);
    }

    m_dangerZoneProcessor.setRules(vecRules);
}

void CPMNMAlgoDetect::setAlgoParamCfg(const Alarm::LongTimeStayDetection_S &stAlgoCfg)
{
    dlog_debug("PMNM_ALGO_DETECT: 设置长时间滞留参数 规则数[%zu]", stAlgoCfg.aRule.size());

    m_stLongTimeStayCfg = stAlgoCfg;

    /* 长时间滞留：仅人目标，停留达阈值[0,100]秒报警 */
    std::vector<RegionRule_S> vecRules;
    for (size_t i = 0; i < stAlgoCfg.aRule.size(); ++i)
    {
        RegionRule_S stRule;
        if (!makeRegionRule(stAlgoCfg.aRule[i].stRegion, stAlgoCfg.aRule[i].nSensitivity,
                            stAlgoCfg.aRule[i].nTimeThreshold, stRule))
        {
            continue;
        }
        vecRules.push_back(stRule);
    }

    m_longTimeStayProcessor.setRules(vecRules);
}

void CPMNMAlgoDetect::setAlgoParamCfg(const Alarm::LoiteringDetection_S &stAlgoCfg)
{
    dlog_debug("PMNM_ALGO_DETECT: 设置人员徘徊检测参数 规则数[%zu]", stAlgoCfg.aRule.size());

    m_stPersonLoiteringCfg = stAlgoCfg;

    /* 人员徘徊：仅人目标，停留达阈值[0,100]秒报警 */
    std::vector<RegionRule_S> vecRules;
    for (size_t i = 0; i < stAlgoCfg.aRule.size(); ++i)
    {
        RegionRule_S stRule;
        if (!makeRegionRule(stAlgoCfg.aRule[i].stRegion, stAlgoCfg.aRule[i].nSensitivity,
                            stAlgoCfg.aRule[i].nTimeThreshold, stRule))
        {
            continue;
        }
        vecRules.push_back(stRule);
    }

    m_personLoiteringProcessor.setRules(vecRules);
}

void CPMNMAlgoDetect::setAlgoParamCfg(const Alarm::DangerZoneStayDetection_S &stAlgoCfg)
{
    dlog_debug("PMNM_ALGO_DETECT: 设置危险区域滞留参数 规则数[%zu]", stAlgoCfg.aRule.size());

    m_stDangerZoneStayCfg = stAlgoCfg;

    /* 危险区域滞留：仅人目标，停留达阈值[0,100]秒报警 */
    std::vector<RegionRule_S> vecRules;
    for (size_t i = 0; i < stAlgoCfg.aRule.size(); ++i)
    {
        RegionRule_S stRule;
        if (!makeRegionRule(stAlgoCfg.aRule[i].stRegion, stAlgoCfg.aRule[i].nSensitivity,
                            stAlgoCfg.aRule[i].nTimeThreshold, stRule))
        {
            continue;
        }
        vecRules.push_back(stRule);
    }

    m_dangerZoneStayProcessor.setRules(vecRules);
}

void CPMNMAlgoDetect::setAlgoParamCfg(const Alarm::KeyAreaAbnormalStayDetection_S &stAlgoCfg)
{
    dlog_debug("PMNM_ALGO_DETECT: 设置重点区域异常滞留参数 规则数[%zu]", stAlgoCfg.aRule.size());

    m_stKeyAreaAbnormalStayCfg = stAlgoCfg;

    /* 重点区域异常滞留：仅人目标，停留达阈值[0,100]秒报警 */
    std::vector<RegionRule_S> vecRules;
    for (size_t i = 0; i < stAlgoCfg.aRule.size(); ++i)
    {
        RegionRule_S stRule;
        if (!makeRegionRule(stAlgoCfg.aRule[i].stRegion, stAlgoCfg.aRule[i].nSensitivity,
                            stAlgoCfg.aRule[i].nTimeThreshold, stRule))
        {
            continue;
        }
        vecRules.push_back(stRule);
    }

    m_keyAreaAbnormalStayProcessor.setRules(vecRules);
}

void CPMNMAlgoDetect::setAlgoParamCfg(const Alarm::RooftopStayDetection_S &stAlgoCfg)
{
    dlog_debug("PMNM_ALGO_DETECT: 设置天台滞留参数 单区域规则");

    m_stRooftopStayCfg = stAlgoCfg;

    /* 天台滞留：仅人目标，单区域，停留达阈值[0,100]秒报警 */
    std::vector<RegionRule_S> vecRules;
    RegionRule_S stRule;
    if (makeRegionRule(stAlgoCfg.stRule.stRegion, stAlgoCfg.stRule.nSensitivity,
                       stAlgoCfg.stRule.nTimeThreshold, stRule))
    {
        vecRules.push_back(stRule);
    }

    m_rooftopStayProcessor.setRules(vecRules);
}

void CPMNMAlgoDetect::setAlgoParamCfg(const Alarm::QueueCrowdingDetection_S &stAlgoCfg)
{
    dlog_debug("PMNM_ALGO_DETECT: 设置排队拥堵检测参数");

    m_stQueueCrowdingCfg = stAlgoCfg;

    /* 排队拥堵：单区域，IOU静止判定 + 人数统计 */
    std::vector<QueueCrowdingRule_S> vecRules;
    QueueCrowdingRule_S stRule;
    stRule.nRuleId = 0;

    Alarm::Region_S stAiRegion;
    if (convertRegionToAiFrame(stAlgoCfg.stRule.stRegion, stAiRegion))
    {
        stRule.stRegion = stAiRegion;
        stRule.nSensitivity = stAlgoCfg.stRule.nSensitivity;
        vecRules.push_back(stRule);
        dlog_info("PMNM_ALGO_DETECT: 排队拥堵区域有效，灵敏度=%u", stRule.nSensitivity);
    }
    else
    {
        dlog_warn("PMNM_ALGO_DETECT: 排队拥堵区域无效");
    }

    m_queueCrowdingProcessor.setRules(vecRules);
}

bool CPMNMAlgoDetect::init()
{
    /* 重复 init 先释放旧组件，保证可重试 */
    if (m_pDetectComponent != nullptr)
    {
        unInit();
    }

    m_pDetectComponent = new CPMNMDetect(PMNM_DETECT_MODEL_CONFIG_PATH);
    if (m_pDetectComponent == nullptr || !m_pDetectComponent->init())
    {
        dlog_error("PMNM_ALGO_DETECT: 纯检测组件初始化失败 [%s]", PMNM_DETECT_MODEL_CONFIG_PATH);
        unInit();
        return false;
    }

    dlog_info("PMNM_ALGO_DETECT: 纯检测组件初始化成功 [%s]", PMNM_DETECT_MODEL_CONFIG_PATH);

    if (!m_oTracker.init(makeTrackerConfig()))
    {
        dlog_error("PMNM_ALGO_DETECT: 跟踪器初始化失败");
    }
    m_climbProcessor.reset();
    m_dangerZoneProcessor.reset();
    m_longTimeStayProcessor.reset();
    m_dangerZoneStayProcessor.reset();
    m_keyAreaAbnormalStayProcessor.reset();
    m_rooftopStayProcessor.reset();
    m_personLoiteringProcessor.reset();
    m_queueCrowdingProcessor.reset();
    m_climbAlarmStateMachine.reset();
    m_dangerZoneAlarmStateMachine.reset();
    m_longTimeStayAlarmStateMachine.reset();
    m_dangerZoneStayAlarmStateMachine.reset();
    m_keyAreaAbnormalStayAlarmStateMachine.reset();
    m_rooftopStayAlarmStateMachine.reset();
    m_personLoiteringAlarmStateMachine.reset();
    m_queueCrowdingAlarmStateMachine.reset();

    return true;
}

bool CPMNMAlgoDetect::unInit()
{
    if (m_pDetectComponent != nullptr)
    {
        m_pDetectComponent->unInit();
        delete m_pDetectComponent;
        m_pDetectComponent = nullptr;
    }
    m_oTracker.uninit();
    return true;
}

void CPMNMAlgoDetect::run()
{
    MediaData_S stMediaData;

    while (m_bRunning.load())
    {
        /* lazy init：组件为空则初始化，失败延迟 1s 重试 */
        if (m_pDetectComponent == nullptr)
        {
            if (!init())
            {
                dlog_error("PMNM_ALGO_DETECT: 等待模型初始化");
                std::unique_lock<std::mutex> lock(m_mutex);
                m_condition.wait_for(lock, std::chrono::seconds(1), [this] {
                    return !m_bRunning.load();
                });
            }
            continue;
        }

        /* 阻塞获取 */
        m_dateQueue.pop(stMediaData, -1);
        if (stMediaData.nSize == 0)
        {
            /* 数据为空（含析构时的唤醒数据） */
            continue;
        }

        /* 帧时间戳（steady_clock ms，单调，判定计时用） */
        int64_t nFrameTsMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

        /* NV12 转 RGB */
        cv::Mat nv12Mat(
            stMediaData.stMediaParam.nVideoHeight * 3 / 2,
            stMediaData.stMediaParam.nVideoWidth,
            CV_8UC1,
            stMediaData.pData.get()
        );
        cv::Mat rgbMat;
        cv::cvtColor(nv12Mat, rgbMat, cv::COLOR_YUV2RGB_NV12);

        /* 检测（组件内部完成 letterbox + rknn 推理 + 坐标还原，阈值回落 json 默认值） */
        CPMNMDetect::InData_S stInData;
        stInData.inMat = rgbMat;
        std::vector<DetectedTarget_S> vecResults;
        if (m_pDetectComponent->process(stInData, vecResults) != 0)
        {
            continue;
        }

        /* 跟踪：检测输出直传，外层下标即类别 */
        std::vector<std::vector<TrackedTarget_S>> vecTracked = m_oTracker.update(vecResults);
        if (vecTracked.size() < static_cast<size_t>(PMNM_TRACK_CLASS_COUNT))
        {
            /* 跟踪器未就绪，外层下标不保证，跳过本帧 */
            continue;
        }

        processRegionStayEvent(m_stClimbCfg.bEnable, Event::Type::CLIMB_DETECTION,
                               m_climbProcessor, m_climbAlarmStateMachine,
                               nFrameTsMs, vecTracked[PMNM_CLASS_PERSON], rgbMat);
        processRegionStayEvent(m_stDangerZoneCfg.bEnable, Event::Type::DANGER_ZONE_APPROACH,
                               m_dangerZoneProcessor, m_dangerZoneAlarmStateMachine,
                               nFrameTsMs, vecTracked[PMNM_CLASS_PERSON], rgbMat);
        processRegionStayEvent(m_stLongTimeStayCfg.bEnable, Event::Type::LONG_TIME_STAY,
                               m_longTimeStayProcessor, m_longTimeStayAlarmStateMachine,
                               nFrameTsMs, vecTracked[PMNM_CLASS_PERSON], rgbMat);
        processRegionStayEvent(m_stDangerZoneStayCfg.bEnable, Event::Type::DANGER_ZONE_STAY,
                               m_dangerZoneStayProcessor, m_dangerZoneStayAlarmStateMachine,
                               nFrameTsMs, vecTracked[PMNM_CLASS_PERSON], rgbMat);
        processRegionStayEvent(m_stKeyAreaAbnormalStayCfg.bEnable, Event::Type::KEY_AREA_ABNORMAL_STAY,
                               m_keyAreaAbnormalStayProcessor, m_keyAreaAbnormalStayAlarmStateMachine,
                               nFrameTsMs, vecTracked[PMNM_CLASS_PERSON], rgbMat);
        processRegionStayEvent(m_stRooftopStayCfg.bEnable, Event::Type::ROOFTOP_STAY,
                               m_rooftopStayProcessor, m_rooftopStayAlarmStateMachine,
                               nFrameTsMs, vecTracked[PMNM_CLASS_PERSON], rgbMat);
        processRegionStayEvent(m_stPersonLoiteringCfg.bEnable, Event::Type::LOITERING_DETECT,
                               m_personLoiteringProcessor, m_personLoiteringAlarmStateMachine,
                               nFrameTsMs, vecTracked[PMNM_CLASS_PERSON], rgbMat);
        processQueueCrowding(nFrameTsMs, vecTracked[PMNM_CLASS_PERSON], rgbMat);

        /* 后厨穿戴规范：子检测器内部判定并直接上报 */
        if (m_bKitchenClothingEnable && m_pKitchenClothingSubDetector)
        {
            m_pKitchenClothingSubDetector->process(rgbMat, vecTracked[PMNM_CLASS_PERSON]);
        }

        /* Group4 行为检测（抽烟/玩手机/摔倒）：子检测器内部判定并直接上报 */
        if (m_bGroup4Enable && m_pGroup4SubDetector)
        {
            m_pGroup4SubDetector->process(rgbMat, vecTracked[PMNM_CLASS_PERSON]);
        }

        /* 人员聚集检测：子检测器内部使用 Group2 模型判定并直接上报 */
        if (m_bCrowdGatheringEnable && m_pCrowdGatheringSubDetector)
        {
            m_pCrowdGatheringSubDetector->process(rgbMat, vecTracked[PMNM_CLASS_PERSON], rgbMat, stMediaData.stMediaParam.nChannel);
        }
        // dlog_debug("PMNM_ALGO_DETECT: 检测到目标[%zu]个", vecResults.size());
    }
}

void CPMNMAlgoDetect::processRegionStayEvent(
    bool bEnable, Event::Type enType,
    CRegionStayProcessor &processor,
    CAlarmStateMachine &stateMachine,
    const int64_t nFrameTsMs,
    const std::vector<TrackedTarget_S> &vecTrackedTarget,
    const cv::Mat &rgbMat)
{
    /* 事件未开启不参与判定 */
    if (!bEnable)
    {
        return;
    }

    std::vector<RegionStayTrigger_S> vecTriggers;
    processor.process(nFrameTsMs, vecTrackedTarget, vecTriggers);

#ifdef ENABLE_TVSDK_SRC
    /* 组装触发上下文，状态机驱动事件开始/结束上报 */
    EventTriggerContext_S stContext;
    stContext.enEventType = enType;
    stContext.nChnId = 0;
    stContext.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    if (!vecTriggers.empty())
    {
        const RegionStayTrigger_S &stTrigger = vecTriggers.front();
        stContext.nTargetId = stTrigger.nTrackId;
        stContext.nObjectType = 1; /* 人 */
        stContext.fConfidence = stTrigger.fConfidence;
        stContext.nLeft   = static_cast<int>(stTrigger.fX1);
        stContext.nTop    = static_cast<int>(stTrigger.fY1);
        stContext.nRight  = static_cast<int>(stTrigger.fX2);
        stContext.nBottom = static_cast<int>(stTrigger.fY2);

        /* 大图整帧编码 小图按目标框裁剪 裁剪异常仅缺小图不影响上报 */
        if (!rgbMat.empty())
        {
            encode_mat_to_tvsdk_image(rgbMat, stContext.stPanoramaImage, JPEG_QUALITY_PANORAMA);

            const int nX1 = std::max(0, stContext.nLeft);
            const int nY1 = std::max(0, stContext.nTop);
            const int nX2 = std::min(rgbMat.cols, stContext.nRight);
            const int nY2 = std::min(rgbMat.rows, stContext.nBottom);
            if (nX2 > nX1 && nY2 > nY1)
            {
                cv::Mat targetMat = rgbMat(cv::Rect(nX1, nY1, nX2 - nX1, nY2 - nY1)).clone();
                encode_mat_to_tvsdk_image(targetMat, stContext.stTargetImage, JPEG_QUALITY_TARGET);
            }
        }
    }
    stateMachine.handleAlarmState(!vecTriggers.empty(), stContext);
#else
    (void)rgbMat;
    stateMachine.handleAlarmState(!vecTriggers.empty(), enType);
#endif
}

void CPMNMAlgoDetect::processQueueCrowding(const int64_t nFrameTsMs, const std::vector<TrackedTarget_S> &vecTrackedTarget,
                                            const cv::Mat &rgbMat)
{
    /* 事件未开启不参与判定 */
    if (!m_stQueueCrowdingCfg.bEnable)
    {
        return;
    }

    std::vector<QueueCrowdingTrigger_S> vecTriggers;
    m_queueCrowdingProcessor.process(nFrameTsMs, vecTrackedTarget, vecTriggers);

#ifdef ENABLE_TVSDK_SRC
    /* 组装触发上下文，状态机驱动事件开始/结束上报 */
    EventTriggerContext_S stContext;
    stContext.enEventType = Event::Type::QUEUE_CROWDING;
    stContext.nChnId = 0;
    stContext.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    if (!vecTriggers.empty())
    {
        const QueueCrowdingTrigger_S &stTrigger = vecTriggers.front();
        stContext.nObjectType = 1; /* 人 */
        dlog_info("PMNM_ALGO_DETECT: 排队拥堵 区域内静止人数 %d >= 阈值 %d，触发报警",
                  stTrigger.nStaticPersonCount, stTrigger.nPersonThreshold);

        /* 统计类事件无单目标框 仅带整帧大图 */
        if (!rgbMat.empty())
        {
            encode_mat_to_tvsdk_image(rgbMat, stContext.stPanoramaImage, JPEG_QUALITY_PANORAMA);
        }
    }

    m_queueCrowdingAlarmStateMachine.handleAlarmState(!vecTriggers.empty(), stContext);
#else
    (void)rgbMat;
    m_queueCrowdingAlarmStateMachine.handleAlarmState(!vecTriggers.empty(), Event::Type::QUEUE_CROWDING);
#endif
}
