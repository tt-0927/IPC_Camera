/**
 * @FilePath     : rv1126b_ipc/main_app/ai_app/detect_mode/access_control_passage_detect/access_control_passage_detect.cpp
 * @Date         : 2026-10-22
 * @Description  : 门禁异常通行检测业务算法实现（取流框架，推理内核复用攀爬检测纯组件）
 */

#include "access_control_passage_detect.hpp"

#include <algorithm>
#include <chrono>
#include <vector>

#include "climb_detect_model.hpp"
#include "common_process.h"
#include "share_data.h"

/* 数据队列容量 */
#define QUEUE_MAX (2)

/* 灵敏度映射置信度门槛范围（与 region_stay_processor 同参数） */
#define SENS_CONFIDENCE_MAX (0.9f)
#define SENS_CONFIDENCE_MIN (0.1f)

CAccessControlPassageDetect::CAccessControlPassageDetect()
 : m_dateQueue(QUEUE_MAX)
{
    m_bRunning.store(true);
    m_thread = std::thread(&CAccessControlPassageDetect::run, this);
}

CAccessControlPassageDetect::~CAccessControlPassageDetect()
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

void CAccessControlPassageDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_bAlgoEnable.load())
    {
        dlog_debug("ACCESS_PASSAGE_DETECT: 算法开关未启用");
        return;
    }

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("ACCESS_PASSAGE_DETECT: 数据队列满了");
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

void CAccessControlPassageDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stCfg.bEnable = stAlgoConfig.nAccessControlAbnormalPassage;

    /* 启用时拉取详细配置并直接下发判定参数 */
    if (stAlgoConfig.nAccessControlAbnormalPassage)
    {
        Alarm::AccessControlAbnormalPassage_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    m_bAlgoEnable.store(stAlgoConfig.nAccessControlAbnormalPassage != 0);
}

void CAccessControlPassageDetect::setAlgoParamCfg(const Alarm::AccessControlAbnormalPassage_S &stAlgoCfg)
{
    dlog_debug("ACCESS_PASSAGE_DETECT: 设置门禁异常通行参数 灵敏度[%u]", stAlgoCfg.stRule.nSensitivity);

    m_stCfg = stAlgoCfg;

    updateRegionPolygon();
    m_fConfGate = sensitivityToConfidence(stAlgoCfg.stRule.nSensitivity);
}

bool CAccessControlPassageDetect::init()
{
    /* 重复 init 先释放旧组件，保证可重试 */
    if (m_pDetectComponent != nullptr)
    {
        unInit();
    }

    m_pDetectComponent = new CClimbDetectModel(CLIMB_DETECT_MODEL_CONFIG_PATH);
    if (m_pDetectComponent == nullptr || !m_pDetectComponent->init())
    {
        dlog_error("ACCESS_PASSAGE_DETECT: 攀爬检测组件初始化失败 [%s]", CLIMB_DETECT_MODEL_CONFIG_PATH);
        unInit();
        return false;
    }

    dlog_info("ACCESS_PASSAGE_DETECT: 攀爬检测组件初始化成功 [%s]", CLIMB_DETECT_MODEL_CONFIG_PATH);

    m_alarmStateMachine.reset();

    return true;
}

bool CAccessControlPassageDetect::unInit()
{
    if (m_pDetectComponent != nullptr)
    {
        m_pDetectComponent->unInit();
        delete m_pDetectComponent;
        m_pDetectComponent = nullptr;
    }
    return true;
}

void CAccessControlPassageDetect::run()
{
    MediaData_S stMediaData;

    while (m_bRunning.load())
    {
        /* lazy init：组件为空则初始化，失败延迟 1s 重试 */
        if (m_pDetectComponent == nullptr)
        {
            if (!init())
            {
                dlog_error("ACCESS_PASSAGE_DETECT: 等待模型初始化");
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
        CClimbDetectModel::InData_S stInData;
        stInData.inMat = rgbMat;
        std::vector<DetectedTarget_S> vecResults;
        if (m_pDetectComponent->process(stInData, vecResults) != 0)
        {
            continue;
        }

        /* 区域判定 + 报警状态机 */
        processPassage(vecResults);
    }
}

void CAccessControlPassageDetect::processPassage(const std::vector<DetectedTarget_S> &vecResults)
{
    /* 事件未开启不参与判定 */
    if (!m_stCfg.bEnable)
    {
        return;
    }

    /* 置信度达门槛且脚点在区域内的首个攀爬目标作为触发目标 */
    const DetectedTarget_S *pstHitTarget = nullptr;
    for (const auto &stResult : vecResults)
    {
        if (stResult.fConfidence < m_fConfGate)
        {
            continue;
        }

        if (!isTargetInRegion(stResult))
        {
            continue;
        }

        pstHitTarget = &stResult;
        break;
    }

    /* 组装触发上下文，状态机驱动事件开始/结束上报 */
    EventTriggerContext_S stContext;
    stContext.enEventType = Event::Type::ACCESS_CONTROL_ABNORMAL_PASSAGE;
    stContext.nChnId = 0;
    stContext.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    if (pstHitTarget != nullptr)
    {
        stContext.nObjectType = 1; /* 人 */
        stContext.fConfidence = pstHitTarget->fConfidence;
        stContext.nLeft   = static_cast<int>(pstHitTarget->fX1);
        stContext.nTop    = static_cast<int>(pstHitTarget->fY1);
        stContext.nRight  = static_cast<int>(pstHitTarget->fX2);
        stContext.nBottom = static_cast<int>(pstHitTarget->fY2);
    }
    m_alarmStateMachine.handleAlarmState(pstHitTarget != nullptr, stContext);
}

void CAccessControlPassageDetect::updateRegionPolygon()
{
    m_vecRegionPts.clear();

    /* 区域无效（未绘制）不参与判定，保持空多边形 */
    Alarm::Region_S stRegion = m_stCfg.stRule.stRegion;
    if (!stRegion.IsValid())
    {
        dlog_info("ACCESS_PASSAGE_DETECT: 区域无效，不参与判定");
        return;
    }

    /* 1920x1080 基准转换到 AI 检测帧分辨率 */
    stRegion.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, PIXEL_WIDTH_AI, PIXEL_HEIGHT_AI);

    size_t nCount = std::min(static_cast<size_t>(stRegion.nPointNum), stRegion.aPoint.size());
    m_vecRegionPts.reserve(nCount);
    for (size_t i = 0; i < nCount; i++)
    {
        m_vecRegionPts.emplace_back(static_cast<int>(stRegion.aPoint[i].fX),
                                    static_cast<int>(stRegion.aPoint[i].fY));
    }
}

float CAccessControlPassageDetect::sensitivityToConfidence(unsigned int nSensitivity)
{
    int nClampedSens = std::clamp(static_cast<int>(nSensitivity), 1, 100);
    float fConfidence = SENS_CONFIDENCE_MAX -
                        (nClampedSens - 1) * (SENS_CONFIDENCE_MAX - SENS_CONFIDENCE_MIN) / (100 - 1);

    return std::clamp(fConfidence, SENS_CONFIDENCE_MIN, SENS_CONFIDENCE_MAX);
}

bool CAccessControlPassageDetect::isTargetInRegion(const DetectedTarget_S &stTarget) const
{
    /* 空多边形（区域无效）不参与判定 */
    if (m_vecRegionPts.empty())
    {
        return false;
    }

    /* 框底部中心点（脚点）判定，与既有区域判定口径一致 */
    cv::Point2f stFoot((stTarget.fX1 + stTarget.fX2) / 2.0f, stTarget.fY2);
    return cv::pointPolygonTest(m_vecRegionPts, stFoot, false) >= 0;
}
