/**
 * @FilePath     : rv1126b_ipc/main_app/ai_app/detect_mode/mouse_algo_detect/mouse_algo_detect.cpp
 * @Date         : 2026-10-23
 * @Description  : 鼠患识别业务算法实现（取流框架，推理内核复用 algorithm_mode 纯检测组件）
 */

#include "mouse_algo_detect.hpp"

#include <algorithm>
#include <chrono>
#include <vector>

#include "common_process.h"
#include "mouse_detect.hpp"
#include "video_define.h"
#include "share_data.h"

/* 数据队列容量 */
#define QUEUE_MAX (2)

/* 灵敏度[1,100] 反向映射置信度区间，灵敏度越高门槛越低（对照 region_stay_processor.cpp） */
#define SENS_CONFIDENCE_MAX (0.9f)
#define SENS_CONFIDENCE_MIN (0.1f)

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

/* 灵敏度[1,100] 反向映射置信度门槛（对照 CRegionStayProcessor::sensitivityToConfidence） */
static float sensitivityToConfidence(unsigned int nSensitivity)
{
    int nClampedSens = std::clamp(static_cast<int>(nSensitivity), 1, 100);
    float fConfidence = SENS_CONFIDENCE_MAX -
                        (nClampedSens - 1) * (SENS_CONFIDENCE_MAX - SENS_CONFIDENCE_MIN) / (100 - 1);

    return std::clamp(fConfidence, SENS_CONFIDENCE_MIN, SENS_CONFIDENCE_MAX);
}

/* 点是否在多边形内（射线法，点数<3 恒为外；对照 region_stay_processor.cpp 同名逻辑） */
static bool isPointInRegion(float fX, float fY, const Alarm::Region_S &stRegion)
{
    size_t nCount = std::min(static_cast<size_t>(stRegion.nPointNum), stRegion.aPoint.size());
    if (nCount < 3)
    {
        return false;
    }

    bool bInside = false;
    for (size_t i = 0, j = nCount - 1; i < nCount; j = i, ++i)
    {
        const Common::PosF_S &stCur = stRegion.aPoint[i];
        const Common::PosF_S &stPre = stRegion.aPoint[j];

        if (((stCur.fY > fY) != (stPre.fY > fY)) &&
            (fX < (stPre.fX - stCur.fX) * (fY - stCur.fY) / (stPre.fY - stCur.fY) + stCur.fX))
        {
            bInside = !bInside;
        }
    }

    return bInside;
}

CMouseAlgoDetect::CMouseAlgoDetect()
 : m_dateQueue(QUEUE_MAX)
{
    m_bRunning.store(true);
    m_thread = std::thread(&CMouseAlgoDetect::run, this);
}

CMouseAlgoDetect::~CMouseAlgoDetect()
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

void CMouseAlgoDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_bAlgoEnable.load())
    {
        dlog_debug("MOUSE_ALGO_DETECT: 算法开关未启用");
        return;
    }

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("MOUSE_ALGO_DETECT: 数据队列满了");
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

void CMouseAlgoDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stAlgoMouseCfg.bEnable = stAlgoConfig.nMouseDetection;

    /* 启用时拉取详细配置并下发 */
    if (stAlgoConfig.nMouseDetection)
    {
        Alarm::MouseDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    m_bAlgoEnable.store(stAlgoConfig.nMouseDetection != 0);

    dlog_info("MOUSE_ALGO_DETECT: 配置更新 鼠患识别[%d]", stAlgoConfig.nMouseDetection);
}

void CMouseAlgoDetect::setAlgoParamCfg(const Alarm::MouseDetection_S &stAlgoCfg)
{
    dlog_debug("MOUSE_ALGO_DETECT: 设置鼠患识别参数 灵敏度[%u]", stAlgoCfg.stRule.nSensitivity);

    m_stAlgoMouseCfg = stAlgoCfg;

    /* 区域由 1920x1080 基准转到 AI 检测帧分辨率，无效区域保持不判定 */
    Alarm::Region_S stRegion;
    if (!convertRegionToAiFrame(stAlgoCfg.stRule.stRegion, stRegion))
    {
        dlog_error("MOUSE_ALGO_DETECT: 检测区域无效，鼠患判定不生效");
    }
    m_stAlgoMouseCfg.stRule.stRegion = stRegion;
}

bool CMouseAlgoDetect::init()
{
    /* 重复 init 先释放旧组件，保证可重试 */
    if (m_pDetectComponent != nullptr)
    {
        unInit();
    }

    m_pDetectComponent = new CMouseDetect(MOUSE_DETECT_MODEL_CONFIG_PATH);
    if (m_pDetectComponent == nullptr || !m_pDetectComponent->init())
    {
        dlog_error("MOUSE_ALGO_DETECT: 纯检测组件初始化失败 [%s]", MOUSE_DETECT_MODEL_CONFIG_PATH);
        unInit();
        return false;
    }

    dlog_info("MOUSE_ALGO_DETECT: 纯检测组件初始化成功 [%s]", MOUSE_DETECT_MODEL_CONFIG_PATH);

    m_mouseAlarmStateMachine.reset();

    return true;
}

bool CMouseAlgoDetect::unInit()
{
    if (m_pDetectComponent != nullptr)
    {
        m_pDetectComponent->unInit();
        delete m_pDetectComponent;
        m_pDetectComponent = nullptr;
    }
    return true;
}

void CMouseAlgoDetect::run()
{
    MediaData_S stMediaData;

    while (m_bRunning.load())
    {
        /* lazy init：组件为空则初始化，失败延迟 1s 重试 */
        if (m_pDetectComponent == nullptr)
        {
            if (!init())
            {
                dlog_error("MOUSE_ALGO_DETECT: 等待模型初始化");
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
        CMouseDetect::InData_S stInData;
        stInData.inMat = rgbMat;
        std::vector<DetectedTarget_S> vecResults;
        if (m_pDetectComponent->process(stInData, vecResults) != 0)
        {
            continue;
        }

        /* 纯帧判定：检测到老鼠活动目标（类别达标 + 置信度达标 + 位于区域内）即触发 */
        processMouseDetect(vecResults);
    }
}

bool CMouseAlgoDetect::processMouseDetect(const std::vector<DetectedTarget_S> &vecResults)
{
    /* 事件未开启不参与判定 */
    if (!m_stAlgoMouseCfg.bEnable)
    {
        return false;
    }

    /* 遍历检测结果：类别过滤 → 置信度过滤 → 区域判定 */
    bool bAlarm = false;
    DetectedTarget_S stHitTarget;
    const float fConfidenceThresh = sensitivityToConfidence(m_stAlgoMouseCfg.stRule.nSensitivity);

    for (size_t i = 0; i < vecResults.size(); ++i)
    {
        const DetectedTarget_S &stTarget = vecResults[i];

        /* 目标过滤：仅老鼠类别参与判定 */
        if (stTarget.nLabel != MOUSE_CLASS_MOUSE)
        {
            continue;
        }

        /* 置信度过滤：低于灵敏度映射门槛的目标不参与判定 */
        if (stTarget.fConfidence < fConfidenceThresh)
        {
            continue;
        }

        /* 区域判定：目标框底部中心位于检测区域内（贴地目标，对照 region_stay_processor 判定点） */
        if (!isPointInRegion((stTarget.fX1 + stTarget.fX2) / 2.0f, stTarget.fY2,
                             m_stAlgoMouseCfg.stRule.stRegion))
        {
            continue;
        }

        stHitTarget = stTarget;
        bAlarm = true;
        break;
    }

    /* 组装触发上下文，状态机驱动事件开始/结束上报 */
    EventTriggerContext_S stContext;
    stContext.enEventType = Event::Type::MOUSE_INFESTATION;
    stContext.nChnId = 0;
    stContext.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    if (bAlarm)
    {
        stContext.nTargetId = -1;  /* 纯帧判定无跟踪，无目标跟踪 ID */
        stContext.nObjectType = 0; /* 老鼠未在目标类型字典中定义，保持未知 */
        stContext.fConfidence = stHitTarget.fConfidence;
        stContext.nLeft   = static_cast<int>(stHitTarget.fX1);
        stContext.nTop    = static_cast<int>(stHitTarget.fY1);
        stContext.nRight  = static_cast<int>(stHitTarget.fX2);
        stContext.nBottom = static_cast<int>(stHitTarget.fY2);
    }
    m_mouseAlarmStateMachine.handleAlarmState(bAlarm, stContext);

    return bAlarm;
}
