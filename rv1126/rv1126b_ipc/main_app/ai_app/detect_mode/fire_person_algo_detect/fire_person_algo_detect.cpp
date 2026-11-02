/**
 * @FilePath     : rv1126b_ipc/main_app/ai_app/detect_mode/fire_person_algo_detect/fire_person_algo_detect.cpp
 * @Date         : 2026-10-14
 * @Description  : 烟火识别、明火检测业务实现
 */

#include "fire_person_algo_detect.hpp"
#include "algo_stream_deal.h"
#include "StatisticsTimer.hpp"
#include "video_define.h" /* PIXEL_WIDTH_1920 等分辨率基准宏 */
#include <unistd.h> /* access */

/* 数据队列 */
#define QUEUE_MAX (2)

CFirePersonAlgoDetect::CFirePersonAlgoDetect()
    : m_dateQueue(QUEUE_MAX)
{
    m_bRunning.store(true);
    m_thread = std::thread(&CFirePersonAlgoDetect::run, this);
}

CFirePersonAlgoDetect::~CFirePersonAlgoDetect()
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
void CFirePersonAlgoDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_stAlgoSmokeFireCfg.bEnable && !m_stAlgoOpenFlameCfg.bEnable && !m_stAlgoOpenFlameLeaveCfg.bEnable)
    {
        dlog_debug("ai_app: 烟火识别-开关未启用");
        return;
    }

    m_nChannelId = stMediaData.stMediaParam.nChannel;

    if (m_RecvManager.handleEvent(m_nChannelId))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("烟火识别-数据队列满了 [%d]", m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

float CFirePersonAlgoDetect::sensitivityToConfidence(int sensitivity, float minConfidence, float maxConfidence)
{
    int clampedSens = std::clamp(sensitivity, 1, 100);

    float confidence = maxConfidence - (clampedSens - 1) * (maxConfidence - minConfidence) / (100 - 1);

    confidence = std::clamp(confidence, minConfidence, maxConfidence);

    return confidence;
}

int CFirePersonAlgoDetect::sensitivityToFrames(int sensitivity, int minFrames, int maxFrames)
{
    if (sensitivity <= 0)
        return maxFrames;
    if (sensitivity >= 100)
        return minFrames;

    // 线性映射
    double ratio  = (100.0 - sensitivity) / 100.0;
    int    frames = minFrames + static_cast<int>(ratio * (maxFrames - minFrames));

    return frames;
}

bool CFirePersonAlgoDetect::init()
{
    if (m_pDetector == nullptr)
    {
        m_pDetector = new CFirePersonDetector(FIRE_PERSON_DETECT_MODEL_CONFIG_PATH);
        if (m_pDetector)
        {
            if (m_pDetector->init())
            {
                dlog_debug("ai_app: 烟火识别算法初始化成功, %s", FIRE_PERSON_DETECT_MODEL_CONFIG_PATH);
                return true;
            }
            delete m_pDetector;
            m_pDetector = nullptr;
            dlog_error("ai_app: 烟火识别算法初始化失败");
        }
    }
    return false;
}

bool CFirePersonAlgoDetect::unInit()
{
    if (m_pDetector != nullptr)
    {
        m_pDetector->unInit();
        delete m_pDetector;
        m_pDetector = nullptr;
    }

    return true;
}

/**
 * @brief 更新算法配置参数
 * @param stAlgoConfig
 */
void CFirePersonAlgoDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stAlgoSmokeFireCfg.bEnable = stAlgoConfig.nEnSmokeFire;
    m_stAlgoOpenFlameCfg.bEnable = stAlgoConfig.nEnOpenFlame;
    m_stAlgoOpenFlameLeaveCfg.bEnable = stAlgoConfig.nEnOpenFlameLeave;

    if (m_stAlgoSmokeFireCfg.bEnable)
    {
        Alarm::SmokeFireDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_stAlgoOpenFlameCfg.bEnable)
    {
        Alarm::OpenFlameDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_stAlgoOpenFlameLeaveCfg.bEnable)
    {
        Alarm::OpenFlameLeaveDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    return;
}

void CFirePersonAlgoDetect::setAlgoParamCfg(const Alarm::SmokeFireDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置烟火识别检测参数");
    m_stAlgoSmokeFireCfg = stAlgoCfg;
}

void CFirePersonAlgoDetect::setAlgoParamCfg(const Alarm::OpenFlameDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置明火识别检测参数");
    m_stAlgoOpenFlameCfg = stAlgoCfg;
    return;
}

void CFirePersonAlgoDetect::setAlgoParamCfg(const Alarm::OpenFlameLeaveDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置明火离人检测参数");
    m_stAlgoOpenFlameLeaveCfg = stAlgoCfg;
    m_llOpenFlameLeaveStartMs = 0;
    return;
}

void CFirePersonAlgoDetect::run()
{
    MediaData_S                   stMediaData;
    std::vector<DetectedTarget_S> vecResults;

    while (m_bRunning.load())
    {
        if (m_pDetector == nullptr)
        {
            if (!init())
            {
                dlog_error("ai_app: 烟火识别等待模型初始化");
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

        CStatisticsTimer runTime("烟火识别完整耗时");

        /* 送分析 */
        if (stMediaData.pData)
        {
            frameRate("烟火识别-分析数据", 5);

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
                if (access("/fire_person_debugImage", F_OK) == 0)
                {
                    dlog_debug("============>debugImage");
                    cv::imwrite("/opt/algo/Image/fire_person_debugImage.jpg", rgbMat);
                }

                CFirePersonDetector::InData_S stInData;
                stInData.inMat               = rgbMat;
                stInData.stParam.fBoxThreshold = -1.0f; /* 用json默认阈值 */
                stInData.stParam.fNmsThreshold  = -1.0f;

                /* 分析数据 */
                {
                    CStatisticsTimer runTime("烟火识别算法耗时");
                    if (m_pDetector->process(stInData, vecResults) == 0)
                    {
                        processDetectResult(vecResults);
                    }
                    else
                    {
                        dlog_error("ai_app: 烟火识别推理失败");
                    }
                }
            }
            else
            {
                dlog_error("ai_app: 烟火识别-图片数据为空");
            }
        }
        else
        {
            dlog_error("ai_app: 烟火识别-获取虚拟地址失败");
        }
    }
}

void CFirePersonAlgoDetect::processDetectResult(const std::vector<DetectedTarget_S> &vecResult)
{
    /* OSD 动态框（命中的烟雾/火焰框，原图坐标） */
    std::vector<Common::RectInfo_S> vstRectInfo;

    /* 烟火识别判定 */
    processSmokeFire(vecResult, vstRectInfo);

    /* 明火识别判定 */
    processOpenFlame(vecResult, vstRectInfo);

    /* 明火离人判定（单区域，条件持续计时） */
    processOpenFlameLeave(vecResult, vstRectInfo);

    if (vstRectInfo.size())
    {
        send_detectionResult_to_osd(m_fullRgbMat.cols, m_fullRgbMat.rows, vstRectInfo);
    }

    return;
}

void CFirePersonAlgoDetect::processSmokeFire(const std::vector<DetectedTarget_S> &vecResult,
                                             std::vector<Common::RectInfo_S> &vstRectInfo)
{
    /* 未启用清零计数后不参与判定 */
    if (!m_stAlgoSmokeFireCfg.bEnable)
    {
        m_nSmokeFireFrameCount = 0;
        return;
    }

    /* 烟火识别：烟雾或火焰任一检出即计数 */
    bool  bDetectSmokeFire = false;
    float fSmokeFireConf   = sensitivityToConfidence(m_stAlgoSmokeFireCfg.stRule.nSensitivity);

    for (const auto &stResult : vecResult)
    {
        bool bAccepted = false;

        if (stResult.nLabel == FIRE_PERSON_CLASS_SMOKE)
        {
            if (stResult.fConfidence > fSmokeFireConf)
            {
                bDetectSmokeFire = true;
                bAccepted = true;
            }
        }
        else if (stResult.nLabel == FIRE_PERSON_CLASS_FIRE)
        {
            /* 火焰同时参与烟火与明火识别，阈值各自独立 */
            if (stResult.fConfidence > fSmokeFireConf)
            {
                bDetectSmokeFire = true;
                bAccepted = true;
            }
        }

        if (bAccepted)
        {
            Common::RectInfo_S stRectInfo;
            stRectInfo.nX1 = (int)stResult.fX1;
            stRectInfo.nY1 = (int)stResult.fY1;
            stRectInfo.nX2 = (int)stResult.fX2;
            stRectInfo.nY2 = (int)stResult.fY2;
            vstRectInfo.push_back(stRectInfo);
        }
    }

    /* 连续帧判定：检出累加，漏检清零 */
    if (bDetectSmokeFire)
    {
        m_nSmokeFireFrameCount++;
    }
    else
    {
        m_nSmokeFireFrameCount = 0;
    }

    bool bSmokeFireAlarm = m_nSmokeFireFrameCount >= sensitivityToFrames(m_stAlgoSmokeFireCfg.stRule.nSensitivity);

    /* 上报事件 */
#ifdef ENABLE_TVSDK_SRC
    EventTriggerContext_S stContext;
    stContext.enEventType = Event::Type_E::SMOKE_FIRE;
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
    m_SmokeFireStateMachine.handleAlarmState(bSmokeFireAlarm, stContext);
#else
    m_SmokeFireStateMachine.handleAlarmState(bSmokeFireAlarm, Event::Type_E::SMOKE_FIRE);
#endif
}

void CFirePersonAlgoDetect::processOpenFlame(const std::vector<DetectedTarget_S> &vecResult,
                                             std::vector<Common::RectInfo_S> &vstRectInfo)
{
    /* 未启用清零计数后不参与判定 */
    if (!m_stAlgoOpenFlameCfg.bEnable)
    {
        m_nFireFrameCount = 0;
        return;
    }

    /* 明火识别：仅火焰 */
    bool  bDetectFire    = false;
    float fOpenFlameConf = sensitivityToConfidence(m_stAlgoOpenFlameCfg.stRule.nSensitivity);

    for (const auto &stResult : vecResult)
    {
        if (stResult.nLabel == FIRE_PERSON_CLASS_FIRE && stResult.fConfidence > fOpenFlameConf)
        {
            bDetectFire = true;

            Common::RectInfo_S stRectInfo;
            stRectInfo.nX1 = (int)stResult.fX1;
            stRectInfo.nY1 = (int)stResult.fY1;
            stRectInfo.nX2 = (int)stResult.fX2;
            stRectInfo.nY2 = (int)stResult.fY2;
            vstRectInfo.push_back(stRectInfo);
        }
    }

    /* 连续帧判定：检出累加，漏检清零 */
    if (bDetectFire)
    {
        m_nFireFrameCount++;
    }
    else
    {
        m_nFireFrameCount = 0;
    }

    bool bFireAlarm = m_nFireFrameCount >= sensitivityToFrames(m_stAlgoOpenFlameCfg.stRule.nSensitivity);

    /* 上报事件 */
#ifdef ENABLE_TVSDK_SRC
    EventTriggerContext_S stContext;
    stContext.enEventType = Event::Type_E::OPEN_FLAME;
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
    m_OpenFlameStateMachine.handleAlarmState(bFireAlarm, stContext);
#else
    m_OpenFlameStateMachine.handleAlarmState(bFireAlarm, Event::Type_E::OPEN_FLAME);
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

void CFirePersonAlgoDetect::processOpenFlameLeave(const std::vector<DetectedTarget_S> &vecResult,
                                                  std::vector<Common::RectInfo_S> &vstRectInfo)
{
    /* 事件未开启不参与判定 */
    if (!m_stAlgoOpenFlameLeaveCfg.bEnable)
    {
        return;
    }

    /* 区域无效不参与判定 */
    const Alarm::Region_S &stRegionCfg = m_stAlgoOpenFlameLeaveCfg.stRule.stRegion;
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

    /* 判定区域内明火与人员（火焰框用中心点，人员框用脚底点），命中框送 OSD */
    float fOpenFlameLeaveConf = sensitivityToConfidence(m_stAlgoOpenFlameLeaveCfg.stRule.nSensitivity);
    bool  bFireInRegion       = false;
    bool  bPersonInRegion     = false;
    for (const auto &stResult : vecResult)
    {
        bool bInFireRegion   = stResult.nLabel == FIRE_PERSON_CLASS_FIRE &&
                               stResult.fConfidence > fOpenFlameLeaveConf &&
                               isPointInRegion((stResult.fX1 + stResult.fX2) / 2.0f,
                                               (stResult.fY1 + stResult.fY2) / 2.0f, stRegion);
        bool bInPersonRegion = stResult.nLabel == FIRE_PERSON_CLASS_PERSON &&
                               isPointInRegion((stResult.fX1 + stResult.fX2) / 2.0f, stResult.fY2, stRegion);

        if (bInFireRegion)
        {
            bFireInRegion = true;
        }
        if (bInPersonRegion)
        {
            bPersonInRegion = true;
        }
        if (bInFireRegion || bInPersonRegion)
        {
            Common::RectInfo_S stRectInfo;
            stRectInfo.nX1 = (int)stResult.fX1;
            stRectInfo.nY1 = (int)stResult.fY1;
            stRectInfo.nX2 = (int)stResult.fX2;
            stRectInfo.nY2 = (int)stResult.fY2;
            vstRectInfo.push_back(stRectInfo);
        }
    }

    /* 区域内有明火且无人持续达阈值触发（0 秒=满足即触发），否则计时清零 */
    bool bAlarm = false;
    if (bFireInRegion && !bPersonInRegion)
    {
        int64_t llNowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::system_clock::now().time_since_epoch())
                              .count();
        if (m_llOpenFlameLeaveStartMs == 0)
        {
            m_llOpenFlameLeaveStartMs = llNowMs;
        }

        int64_t llThresholdMs = static_cast<int64_t>(m_stAlgoOpenFlameLeaveCfg.stRule.nTimeThreshold) * 1000;
        bAlarm = (llNowMs - m_llOpenFlameLeaveStartMs) >= llThresholdMs;
    }
    else
    {
        m_llOpenFlameLeaveStartMs = 0;
    }

    /* 上报事件 */
#ifdef ENABLE_TVSDK_SRC
    EventTriggerContext_S stContext;
    stContext.enEventType = Event::Type_E::OPEN_FLAME_LEAVE;
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
    m_OpenFlameLeaveStateMachine.handleAlarmState(bAlarm, stContext);
#else
    m_OpenFlameLeaveStateMachine.handleAlarmState(bAlarm, Event::Type_E::OPEN_FLAME_LEAVE);
#endif
}
