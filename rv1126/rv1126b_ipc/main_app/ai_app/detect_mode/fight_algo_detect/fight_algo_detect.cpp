/**
 * @FilePath     : rv1126b_ipc/main_app/ai_app/detect_mode/fight_algo_detect/fight_algo_detect.cpp
 * @Date         : 2026-10-21
 * @Description  : 打架斗殴检测业务实现
 */

#include "fight_algo_detect.hpp"
#include "fight_detect.hpp"
#include "algo_stream_deal.h"
#include "StatisticsTimer.hpp"
#include "video_define.h" /* PIXEL_WIDTH_1920 等分辨率基准宏 */
#include <unistd.h> /* access */

/* 数据队列 */
#define QUEUE_MAX (2)

/* 点是否在多边形内（定义见文件末尾，前置声明供 processFight 使用） */
static bool isPointInRegion(float fX, float fY, const Alarm::Region_S &stRegion);

CFightAlgoDetect::CFightAlgoDetect()
    : m_dateQueue(QUEUE_MAX)
{
    m_bRunning.store(true);
    m_thread = std::thread(&CFightAlgoDetect::run, this);
}

CFightAlgoDetect::~CFightAlgoDetect()
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

/* 接受媒体数据 */
void CFightAlgoDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_stAlgoFightCfg.bEnable)
    {
        dlog_debug("ai_app: 打架斗殴检测-开关未启用");
        return;
    }

    m_nChannelId = stMediaData.stMediaParam.nChannel;

    if (m_RecvManager.handleEvent(m_nChannelId))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("打架斗殴检测-数据队列满了 [%d]", m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

float CFightAlgoDetect::sensitivityToConfidence(int sensitivity, float minConfidence, float maxConfidence)
{
    int clampedSens = std::clamp(sensitivity, 1, 100);

    float confidence = maxConfidence - (clampedSens - 1) * (maxConfidence - minConfidence) / (100 - 1);

    confidence = std::clamp(confidence, minConfidence, maxConfidence);

    return confidence;
}

bool CFightAlgoDetect::init()
{
    if (m_pDetector == nullptr)
    {
        m_pDetector = new FightDetect(FIGHT_MODEL_CONFIG_PATH);
        if (m_pDetector)
        {
            if (m_pDetector->init())
            {
                dlog_debug("ai_app: 打架斗殴初始化成功, %s", FIGHT_MODEL_CONFIG_PATH);
                return true;
            }
            delete m_pDetector;
            m_pDetector = nullptr;
            dlog_error("ai_app: 打架斗殴初始化失败");
        }
    }
    return false;
}

bool CFightAlgoDetect::unInit()
{
    if (m_pDetector != nullptr)
    {
        m_pDetector->unInit();
        delete m_pDetector;
        m_pDetector = nullptr;
    }

    return true;
}

void CFightAlgoDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stAlgoFightCfg.bEnable = stAlgoConfig.nEnFight;

    if (m_stAlgoFightCfg.bEnable)
    {
        Alarm::FightDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
    return;
}

void CFightAlgoDetect::setAlgoParamCfg(const Alarm::FightDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置打架斗殴检测参数");
    m_stAlgoFightCfg = stAlgoCfg;
    m_llFightStartMs = 0;
    return;
}

void CFightAlgoDetect::run()
{
    MediaData_S                   stMediaData;
    std::vector<DetectedTarget_S> vecResults;

    while (m_bRunning.load())
    {
        if (m_pDetector == nullptr)
        {
            if (!init())
            {
                dlog_error("ai_app: 打架斗殴等待模型初始化");
                /* 延迟等待 1s */
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
            /* 数据为空 */
            continue;
        }

        CStatisticsTimer runTime("打架斗殴识别完整耗时");

        /* 送分析 */
        if (stMediaData.pData)
        {
            frameRate("打架斗殴-分析数据", 5);

            cv::Mat i420Mat(
                stMediaData.stMediaParam.nVideoHeight * 3 / 2,
                stMediaData.stMediaParam.nVideoWidth,
                CV_8UC1,
                stMediaData.pData.get());

            /* rgb格式转换（全分辨率，letterbox由组件内部处理） */
            cv::Mat rgbMat;
            cv::cvtColor(i420Mat, rgbMat, cv::COLOR_YUV2RGB_NV12);

            if (!rgbMat.empty())
            {
                m_fullRgbMat = rgbMat.clone();
                // if (access("/fire_person_debugImage", F_OK) == 0)
                // {
                //     dlog_debug("");
                //     cv::imwrite("/opt/algo/Image/fire_person_debugImage.jpg", rgbMat);
                // }

                FightDetect::InData_S stInData;
                stInData.inMat               = rgbMat;
                stInData.stParam.fBoxThreshold = -1.0f; /* 用json默认阈值 */
                stInData.stParam.fNmsThreshold  = -1.0f;

                /* 分析数据 */
                {
                    CStatisticsTimer runTime("打架斗殴算法耗时");
                    if (m_pDetector->process(stInData, vecResults) == 0)
                    {
                        processDetectResult(vecResults);
                    }
                    else
                    {
                        dlog_error("ai_app: 打架斗殴推理失败");
                    }
                }
            }
            else
            {
                dlog_error("ai_app: 打架斗殴-图片数据为空");
            }
        }
        else
        {
            dlog_error("ai_app: 打架斗殴-获取虚拟地址失败");
        }
    }
}

void CFightAlgoDetect::processDetectResult(const std::vector<DetectedTarget_S> &vecResult)
{
    /* OSD 动态框（命中的烟雾/火焰框，原图坐标） */
    std::vector<Common::RectInfo_S> vstRectInfo;

    /* 烟火识别判定 */
    processFight(vecResult, vstRectInfo);

    if (vstRectInfo.size())
    {
        send_detectionResult_to_osd(m_fullRgbMat.cols, m_fullRgbMat.rows, vstRectInfo);
    }

    return;
}

void CFightAlgoDetect::processFight(const std::vector<DetectedTarget_S> &vecResult,
                                             std::vector<Common::RectInfo_S> &vstRectInfo)
{
    /* 未启用不参与判定 */
    if (!m_stAlgoFightCfg.bEnable)
    {
        return;
    }

    /* 区域无效不参与判定 */
    const Alarm::Region_S &stRegionCfg = m_stAlgoFightCfg.stRule.stRegion;
    if (!stRegionCfg.IsValid())
    {
        return;
    }

    /* 区域坐标 1920x1080 基准转到当前帧分辨率 */
    Alarm::Region_S stRegion = stRegionCfg;
    if (m_fullRgbMat.empty() ||
        !stRegion.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_fullRgbMat.cols, m_fullRgbMat.rows))
    {
        return;
    }

    /* 打架检出：过滤类别+置信度+区域 */
    bool  bDetectFight = false;
    float fFightConf   = sensitivityToConfidence(m_stAlgoFightCfg.stRule.nSensitivity);
    const DetectedTarget_S *pstHitTarget = nullptr; /* 区域内首个打架目标 */

    for (const auto &stResult : vecResult)
    {
        if (stResult.nLabel == FIGHT_CLASS_FIGHT && stResult.fConfidence > fFightConf)
        {
            /* 框中心点在区域内 */
            float fCenterX = (stResult.fX1 + stResult.fX2) / 2.0f;
            float fCenterY = (stResult.fY1 + stResult.fY2) / 2.0f;

            if (isPointInRegion(fCenterX, fCenterY, stRegion))
            {
                bDetectFight = true;
                if (pstHitTarget == nullptr)
                {
                    pstHitTarget = &stResult;
                }

                Common::RectInfo_S stRectInfo;
                stRectInfo.nX1 = (int)stResult.fX1;
                stRectInfo.nY1 = (int)stResult.fY1;
                stRectInfo.nX2 = (int)stResult.fX2;
                stRectInfo.nY2 = (int)stResult.fY2;
                vstRectInfo.push_back(stRectInfo);
            }
        }
    }

    /* 连续时间判定：检出计时，漏检清零 */
    bool bAlarm = false;
    if (bDetectFight)
    {
        int64_t llNowMs = std::chrono::duration_cast<std::chrono::milliseconds>
                            (std::chrono::system_clock::now().time_since_epoch()).count();

        if (m_llFightStartMs == 0)
        {
            m_llFightStartMs = llNowMs;
        }

        int64_t thresholdMs = m_stAlgoFightCfg.stRule.nTimeThreshold * 1000;
        bAlarm = (llNowMs - m_llFightStartMs) >= thresholdMs;
    }
    else
    {
        m_llFightStartMs = 0;
    }

    /* 上报事件 */
#ifdef ENABLE_TVSDK_SRC
    EventTriggerContext_S stContext;
    stContext.enEventType = Event::Type_E::FIGHT_DETECTION;
    stContext.nChnId      = m_nChannelId;
    stContext.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::system_clock::now().time_since_epoch())
                                .count();
    if (!m_fullRgbMat.empty())
    {
        auto pPayload = std::make_shared<EventTvSdkPayload_S>();
        pPayload->enType = get_tvsdk_payload_type(stContext.enEventType);
        if (encode_mat_to_tvsdk_image(m_fullRgbMat, pPayload->stPanoramaImage, JPEG_QUALITY_PANORAMA))
        {
            stContext.pTvSdkPayload = pPayload;
        }
    }
    /* 报警帧补目标框与小图 裁剪异常仅缺小图不影响上报 */
    if (bAlarm && pstHitTarget != nullptr && !m_fullRgbMat.empty())
    {
        stContext.nObjectType = 1; /* 人 */
        stContext.fConfidence = pstHitTarget->fConfidence;
        stContext.nLeft   = (int)pstHitTarget->fX1;
        stContext.nTop    = (int)pstHitTarget->fY1;
        stContext.nRight  = (int)pstHitTarget->fX2;
        stContext.nBottom = (int)pstHitTarget->fY2;

        const int nX1 = std::max(0, stContext.nLeft);
        const int nY1 = std::max(0, stContext.nTop);
        const int nX2 = std::min(m_fullRgbMat.cols, stContext.nRight);
        const int nY2 = std::min(m_fullRgbMat.rows, stContext.nBottom);
        if (nX2 > nX1 && nY2 > nY1)
        {
            cv::Mat targetMat = m_fullRgbMat(cv::Rect(nX1, nY1, nX2 - nX1, nY2 - nY1)).clone();
            encode_mat_to_tvsdk_image(targetMat, stContext.stTargetImage, JPEG_QUALITY_TARGET);
        }
    }
    m_FightMachine.handleAlarmState(bAlarm, stContext);
#else
    m_FightMachine.handleAlarmState(bAlarm, Event::Type_E::FIGHT_DETECTION);
#endif
}

/* 点是否在多边形内（射线法，点数<3 恒为外） */
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