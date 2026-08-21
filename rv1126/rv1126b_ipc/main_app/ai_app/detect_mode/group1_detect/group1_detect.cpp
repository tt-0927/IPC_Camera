/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-01-07 11:38:52
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-05-07 17:35:52
 * @FilePath: /1126/rv1126b_ipc/main_app/ai_app/detect_mode/group1_detect/group1_detect.cpp
 * @Description: notHelmet(未戴安全帽)、helmet(安全帽)、reflective(反光衣)、safetyRope(安全绳)、exposedSoil(泥土裸露) 检测相关
 */

#include "group1_detect.hpp"
#include "algo_stream_deal.h"
#include "StatisticsTimer.hpp"
#include "SaveImage.hpp"

/* 数据队列 */
#define QUEUE_MAX (2)

CGroup1Detect::CGroup1Detect()
    : m_dateQueue(QUEUE_MAX)
{
    m_bRunning.store(true);
    m_thread = std::thread(&CGroup1Detect::run, this);
}

CGroup1Detect::~CGroup1Detect()
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
void CGroup1Detect::recvMediaData(MediaData_S stMediaData)
{
    m_nChannelId = stMediaData.stMediaParam.nChannel;

    if (!m_stAlgoSafetyHelmetCfg.bEnable && m_stAlgoReflectiveClothingCfg.bEnable && !m_stAlgoHighAltitudeSeatbeltCfg.bEnable && m_stAlgoBareSoiletCfg.bEnable)
    {
        dlog_debug("ai_app:  模型组合1识别-开关未启用");
        return;
    }

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error(" 模型组合1识别-数据队列满了 [%d]", m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

float CGroup1Detect::sensitivityToConfidence(int sensitivity, float minConfidence, float maxConfidence)
{
    int clampedSens = std::clamp(sensitivity, 1, 100);

    float confidence = maxConfidence - (clampedSens - 1) * (maxConfidence - minConfidence) / (100 - 1);

    confidence = std::clamp(confidence, minConfidence, maxConfidence);

    return confidence;
}

int CGroup1Detect::sensitivityToFrames(int sensitivity, int minFrames, int maxFrames)
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

bool CGroup1Detect::init()
{
    if (!m_pHandle)
    {
        Group1Detect_NS::InParam_S stInParam;
        stInParam.strModelPath = "/opt/cam/model/group1.json";
        stInParam.bDebug       = false;

        m_pHandle = new Group1Detect_NS::CGroup1DetectV1_0(stInParam);
        if (m_pHandle)
        {
            if (m_pHandle->init())
            {
                dlog_debug("ai_app:  模型组合1识别算法初始化成功, %s", stInParam.strModelPath.c_str());

                return true;
            }
            else
            {
                delete m_pHandle;
                m_pHandle = nullptr;
                dlog_debug(" 模型组合1识别算法初始化失败");
            }
        }
    }
    return false;
}

bool CGroup1Detect::unInit()
{
    if (m_pHandle)
    {
        delete m_pHandle;
        m_pHandle = nullptr;
    }

    return true;
}

/**
 * @brief 更新算法配置参数
 * @param stAlgoConfig
 */
void CGroup1Detect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stAlgoSafetyHelmetCfg.bEnable         = stAlgoConfig.nEnSafetyHelmet;
    m_stAlgoReflectiveClothingCfg.bEnable   = stAlgoConfig.nEnReflectiveClothing;
    m_stAlgoHighAltitudeSeatbeltCfg.bEnable = stAlgoConfig.nEnHighAltitudeSeatbelt;
    m_stAlgoBareSoiletCfg.bEnable           = stAlgoConfig.nEnBareSoil;

    if (m_stAlgoSafetyHelmetCfg.bEnable)
    {
        Alarm::SafetyHelmetDection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_stAlgoReflectiveClothingCfg.bEnable)
    {
        Alarm::ReflectiveClothingDection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_stAlgoHighAltitudeSeatbeltCfg.bEnable)
    {
        Alarm::HighAltitudeSeatbeltDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_stAlgoBareSoiletCfg.bEnable)
    {
        Alarm::BareSoiletDection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    return;
}

void CGroup1Detect::setAlgoParamCfg(const Alarm::SafetyHelmetDection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置安全帽识别参数");
    m_stAlgoSafetyHelmetCfg = stAlgoCfg;
    return;
}

void CGroup1Detect::setAlgoParamCfg(const Alarm::ReflectiveClothingDection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置反光衣识别参数");
    m_stAlgoReflectiveClothingCfg = stAlgoCfg;
    return;
}

void CGroup1Detect::setAlgoParamCfg(const Alarm::HighAltitudeSeatbeltDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置高空安全带识别参数");
    m_stAlgoHighAltitudeSeatbeltCfg = stAlgoCfg;
    return;
}

void CGroup1Detect::setAlgoParamCfg(const Alarm::BareSoiletDection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置泥土裸露识别参数");
    m_stAlgoBareSoiletCfg = stAlgoCfg;
    return;
}

void CGroup1Detect::run()
{
    MediaData_S                            stMediaData;
    std::vector<Group1Detect_NS::Result_S> vecResult;
    cv::Mat                                Desframe(m_nHeight, m_nWidth, CV_8UC3);

    while (m_bRunning.load())
    {
        if (!m_pHandle)
        {
            if (!init())
            {
                dlog_error("等待 模型组合1识别初始化");
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

        CStatisticsTimer runTime(" 模型组合1识别完整耗时");

        /* 送分析 */
        if (stMediaData.pData)
        {
            frameRate(" 模型组合1识别-分析数据", 5);

            Group1Detect_NS::InData_S  stInData{};
            Group1Detect_NS::OutData_S stOutData;

            // 利用 RGA  YUV 转 RGB，裁剪,供模型推理
            bool ai_rga_ok = rga_image_transform(
                stMediaData.pData.get(),
                stMediaData.stMediaParam.nVideoWidth,
                stMediaData.stMediaParam.nVideoHeight,
                RK_FORMAT_YCbCr_420_SP,  // NV12
                Desframe.data,
                m_nWidth,
                m_nHeight,
                RK_FORMAT_RGB_888);

            if (ai_rga_ok)
            {
                stInData.inMat = Desframe;
            }
            else
            {
                cv::Mat i420Mat(
                    stMediaData.stMediaParam.nVideoHeight * 3 / 2,
                    stMediaData.stMediaParam.nVideoWidth,
                    CV_8UC1,
                    stMediaData.pData.get());

                /* rgb格式转换 */
                cv::Mat rgbMat;
                cv::cvtColor(i420Mat, rgbMat, cv::COLOR_YUV2RGB_NV12);
                m_fullRgbMat = rgbMat.clone();

                /* 分辨率大小转换 */
                cv::resize(
                    rgbMat,
                    stInData.inMat,
                    cv::Size(m_nWidth, m_nHeight),
                    0,
                    0,
                    cv::INTER_LINEAR);
            }

            // cv::rotate(stInData.inMat, stInData.inMat, cv::ROTATE_180);

            if (!stInData.inMat.empty())
            {
                if (access("/group1Detect_debugImage", F_OK) == 0)
                {
                    dlog_debug("============>debugImage");
                    cv::imwrite("/opt/algo/Image/group1Detect_debugImage.jpg", stInData.inMat);
                }

                /* 安全帽识别 */
                if (m_stAlgoSafetyHelmetCfg.bEnable)
                {
                    stInData.stParam.stSafetyHelmetDetectParam.bEnable      = true;
                    stInData.stParam.stSafetyHelmetDetectParam.fConfidence  = sensitivityToConfidence(m_stAlgoSafetyHelmetCfg.stRule.nSensitivity);
                    stInData.stParam.stSafetyHelmetDetectParam.nDetectFrame = sensitivityToFrames(m_stAlgoSafetyHelmetCfg.stRule.nSensitivity);
                    // printf(" [%s][%d]=== 安全帽识别 %d -> %f %d\n", __FILE__, __LINE__, m_stAlgoSafetyHelmetCfg.stRule.nSensitivity, stInData.stParam.stSafetyHelmetDetectParam.fConfidence, stInData.stParam.stSafetyHelmetDetectParam.nDetectFrame);
                }

                /* 反光衣识别 */
                if (m_stAlgoReflectiveClothingCfg.bEnable)
                {
                    stInData.stParam.stReflectiveClothingParam.bEnable      = true;
                    stInData.stParam.stReflectiveClothingParam.fConfidence  = sensitivityToConfidence(m_stAlgoReflectiveClothingCfg.stRule.nSensitivity);
                    stInData.stParam.stReflectiveClothingParam.nDetectFrame = sensitivityToFrames(m_stAlgoReflectiveClothingCfg.stRule.nSensitivity);
                    // printf(" [%s][%d]=== 反光衣识别 %d -> %f %d\n", __FILE__, __LINE__, m_stAlgoReflectiveClothingCfg.stRule.nSensitivity, stInData.stParam.stReflectiveClothingParam.fConfidence, stInData.stParam.stReflectiveClothingParam.nDetectFrame);
                }

                /* 高空安全带识别 */
                if (m_stAlgoHighAltitudeSeatbeltCfg.bEnable)
                {
                    stInData.stParam.stHighAltitudeSeatbeltParam.bEnable      = true;
                    stInData.stParam.stHighAltitudeSeatbeltParam.fConfidence  = sensitivityToConfidence(m_stAlgoHighAltitudeSeatbeltCfg.stRule.nSensitivity);
                    stInData.stParam.stHighAltitudeSeatbeltParam.nDetectFrame = sensitivityToFrames(m_stAlgoHighAltitudeSeatbeltCfg.stRule.nSensitivity);
                    // printf(" [%s][%d]=== 高空安全带识别 %d -> %f %d\n", __FILE__, __LINE__, m_stAlgoHighAltitudeSeatbeltCfg.stRule.nSensitivity, stInData.stParam.stHighAltitudeSeatbeltParam.fConfidence, stInData.stParam.stHighAltitudeSeatbeltParam.nDetectFrame);
                }

                /* 泥土裸露 */
                if (m_stAlgoBareSoiletCfg.bEnable)
                {
                    stInData.stParam.stBareSoiletParam.bEnable      = true;
                    stInData.stParam.stBareSoiletParam.fConfidence  = sensitivityToConfidence(m_stAlgoBareSoiletCfg.stRule.nSensitivity);
                    stInData.stParam.stBareSoiletParam.nDetectFrame = sensitivityToFrames(m_stAlgoBareSoiletCfg.stRule.nSensitivity);
                    // printf(" [%s][%d]=== 泥土裸露 %d -> %f %d\n", __FILE__, __LINE__, m_stAlgoBareSoiletCfg.stRule.nSensitivity, stInData.stParam.stBareSoiletParam.fConfidence, stInData.stParam.stBareSoiletParam.nDetectFrame);
                }

                /* 分析数据 */
                {
                    CStatisticsTimer runTime(" 模型组合1识别算法耗时");
                    m_pHandle->process(stInData, vecResult, &stOutData);
                    /* 检测后处理 */
                    processGroup1Detect(stOutData);
                    /* 相关事件动态分析 */
                    dynamicAnalysis(vecResult);
                }
            }
            else
            {
                dlog_error("ai_app: 图片数据为空");
            }
        }
        else
        {
            dlog_error("ai_app:  模型组合1识别-获取虚拟地址失败");
        }
    }
}

int CGroup1Detect::dynamicAnalysis(const std::vector<Group1Detect_NS::Result_S> &vecResult)
{
    std::vector<Common::RectInfo_S> vstRectInfo;

    for (auto &stResult : vecResult)
    {
        Common::RectInfo_S stRectInfo;
        stRectInfo.nX1 = (int)stResult.fX1;
        stRectInfo.nY1 = (int)stResult.fY1;
        stRectInfo.nX2 = (int)(stResult.fX2);
        stRectInfo.nY2 = (int)(stResult.fY2);
        vstRectInfo.push_back(stRectInfo);
    }
    if (vstRectInfo.size())
    {
        send_detectionResult_to_osd(m_nWidth, m_nHeight, vstRectInfo);
    }
    return 0;
}

void CGroup1Detect::processGroup1Detect(const Group1Detect_NS::OutData_S &stOutData)
{
    if (m_stAlgoSafetyHelmetCfg.bEnable)
    {
        /* 上报事件 */
#ifdef ENABLE_TVSDK_SRC
        bool bIsAlarm = stOutData.bSafetyHelmet;
        EventTriggerContext_S stContext;
        stContext.enEventType = Event::Type_E::SAFETY_HELMET;
        stContext.nChnId = m_nChannelId;
        stContext.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if (!m_fullRgbMat.empty()) {
            auto pPayload = std::make_shared<EventTvSdkPayload_S>();
            pPayload->enType = get_tvsdk_payload_type(stContext.enEventType);
            if (encode_mat_to_tvsdk_image(m_fullRgbMat, pPayload->stPanoramaImage)) {
                stContext.pTvSdkPayload = pPayload;
            }
        }
        m_SafetyHelmetStateMachine.handleAlarmState(bIsAlarm, stContext);
#else
        m_SafetyHelmetStateMachine.handleAlarmState(stOutData.bSafetyHelmet, Event::Type_E::SAFETY_HELMET);
#endif
    }
    if (m_stAlgoReflectiveClothingCfg.bEnable)
    {
        /* 上报事件 */
#ifdef ENABLE_TVSDK_SRC
        bool bIsAlarm = stOutData.bReflectiveClothing;
        EventTriggerContext_S stContext;
        stContext.enEventType = Event::Type_E::REFLECTIVE_CLOTHING;
        stContext.nChnId = m_nChannelId;
        stContext.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if (!m_fullRgbMat.empty()) {
            auto pPayload = std::make_shared<EventTvSdkPayload_S>();
            pPayload->enType = get_tvsdk_payload_type(stContext.enEventType);
            if (encode_mat_to_tvsdk_image(m_fullRgbMat, pPayload->stPanoramaImage)) {
                stContext.pTvSdkPayload = pPayload;
            }
        }
        m_ReflectiveClothingStateMachine.handleAlarmState(bIsAlarm, stContext);
#else
        m_ReflectiveClothingStateMachine.handleAlarmState(stOutData.bReflectiveClothing, Event::Type_E::REFLECTIVE_CLOTHING);
#endif
    }
    if (m_stAlgoHighAltitudeSeatbeltCfg.bEnable)
    {
        /* 上报事件 */
#ifdef ENABLE_TVSDK_SRC
        bool bIsAlarm = stOutData.bHighAltitudeSeatbelt;
        EventTriggerContext_S stContext;
        stContext.enEventType = Event::Type_E::HIGH_ALTITUDE_SEATBELT;
        stContext.nChnId = m_nChannelId;
        stContext.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if (!m_fullRgbMat.empty()) {
            auto pPayload = std::make_shared<EventTvSdkPayload_S>();
            pPayload->enType = get_tvsdk_payload_type(stContext.enEventType);
            if (encode_mat_to_tvsdk_image(m_fullRgbMat, pPayload->stPanoramaImage)) {
                stContext.pTvSdkPayload = pPayload;
            }
        }
        m_HighAltitudeSeatbeltStateMachine.handleAlarmState(bIsAlarm, stContext);
#else
        m_HighAltitudeSeatbeltStateMachine.handleAlarmState(stOutData.bHighAltitudeSeatbelt, Event::Type_E::HIGH_ALTITUDE_SEATBELT);
#endif
    }
    if (m_stAlgoBareSoiletCfg.bEnable)
    {
        /* 上报事件 */
#ifdef ENABLE_TVSDK_SRC
        bool bIsAlarm = stOutData.bBareSoilet;
        EventTriggerContext_S stContext;
        stContext.enEventType = Event::Type_E::BARE_SOIL;
        stContext.nChnId = m_nChannelId;
        stContext.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if (!m_fullRgbMat.empty()) {
            auto pPayload = std::make_shared<EventTvSdkPayload_S>();
            pPayload->enType = get_tvsdk_payload_type(stContext.enEventType);
            if (encode_mat_to_tvsdk_image(m_fullRgbMat, pPayload->stPanoramaImage)) {
                stContext.pTvSdkPayload = pPayload;
            }
        }
        m_BareSoilStateMachine.handleAlarmState(bIsAlarm, stContext);
#else
        m_BareSoilStateMachine.handleAlarmState(stOutData.bBareSoilet, Event::Type_E::BARE_SOIL);
#endif
    }
    return;
}
