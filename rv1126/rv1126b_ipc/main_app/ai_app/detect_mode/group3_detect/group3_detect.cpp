/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-01-08 19:22:49
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-01-29 11:35:22
 * @FilePath: /1126/rv1126b_ipc/main_app/ai_app/detect_mode/group3_detect/group3_detect.cpp
 * @Description: smoke(烟雾)、fire(火焰)、Overflow(垃圾满溢)、expose(垃圾暴露)、Complete(井盖完好)、Damaged(井盖破损)、Lost(井盖丢失)、Uncovered(未盖井盖)、BreakoutOfOuterEdge(井盖外边沿破损)、WaterAccumulation(道路积水)
 */

#include "group3_detect.hpp"
#include "algo_stream_deal.h"
#include "StatisticsTimer.hpp"
#include "SaveImage.hpp"

/* 数据队列 */
#define QUEUE_MAX (2)

CGroup3Detect::CGroup3Detect()
    : m_dateQueue(QUEUE_MAX)
{
    m_bRunning.store(true);
    m_thread = std::thread(&CGroup3Detect::run, this);
}

CGroup3Detect::~CGroup3Detect()
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
void CGroup3Detect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_stAlgoSmokeFireCfg.bEnable && !m_stAlgoOpenFlameCfg.bEnable && !m_stAlgoGarbageExposureCfg.bEnable && !m_stAlgoGarbageOverflowCfg.bEnable && !m_stAlgoManholeCoverAbnormalCfg.bEnable && !m_stAlgoRoadPondingCfg.bEnable)
    {
        dlog_debug("ai_app:  模型组合3识别-开关未启用");
        return;
    }

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error(" 模型组合3识别-数据队列满了 [%d]", m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

float CGroup3Detect::sensitivityToConfidence(int sensitivity, float minConfidence, float maxConfidence)
{
    int clampedSens = std::clamp(sensitivity, 1, 100);

    float confidence = maxConfidence - (clampedSens - 1) * (maxConfidence - minConfidence) / (100 - 1);

    confidence = std::clamp(confidence, minConfidence, maxConfidence);

    return confidence;
}

int CGroup3Detect::sensitivityToFrames(int sensitivity, int minFrames, int maxFrames)
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

bool CGroup3Detect::init()
{
    if (!m_pHandle)
    {
        Group3Detect_NS::InParam_S stInParam;
        stInParam.strModelPath = "/opt/cam/model/group3.json";
        stInParam.bDebug       = false;

        m_pHandle = new Group3Detect_NS::CGroup3DetectV1_0(stInParam);
        if (m_pHandle)
        {
            if (m_pHandle->init())
            {
                dlog_debug("ai_app:  模型组合3识别算法初始化成功, %s", stInParam.strModelPath.c_str());

                return true;
            }
            else
            {
                delete m_pHandle;
                m_pHandle = nullptr;
                dlog_debug(" 模型组合3识别算法初始化失败");
            }
        }
    }
    return false;
}

bool CGroup3Detect::unInit()
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
void CGroup3Detect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stAlgoSmokeFireCfg.bEnable            = stAlgoConfig.nEnSmokeFire;
    m_stAlgoOpenFlameCfg.bEnable            = stAlgoConfig.nEnOpenFlame;
    m_stAlgoGarbageExposureCfg.bEnable      = stAlgoConfig.nEnGarbageExposure;
    m_stAlgoGarbageOverflowCfg.bEnable      = stAlgoConfig.nEnGarbageOverflow;
    m_stAlgoManholeCoverAbnormalCfg.bEnable = stAlgoConfig.nEnManholeCoverAbnormal;
    m_stAlgoRoadPondingCfg.bEnable          = stAlgoConfig.nEnRoadPonding;

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

    if (m_stAlgoGarbageExposureCfg.bEnable)
    {
        Alarm::GarbageExposureDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_stAlgoGarbageOverflowCfg.bEnable)
    {
        Alarm::GarbageOverflowDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_stAlgoManholeCoverAbnormalCfg.bEnable)
    {
        Alarm::ManholeCoverAbnormalDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_stAlgoRoadPondingCfg.bEnable)
    {
        Alarm::RoadPondingDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    return;
}

void CGroup3Detect::setAlgoParamCfg(const Alarm::SmokeFireDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置烟火检测参数");
    m_stAlgoSmokeFireCfg = stAlgoCfg;
}

void CGroup3Detect::setAlgoParamCfg(const Alarm::OpenFlameDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置明火检测参数");
    m_stAlgoOpenFlameCfg = stAlgoCfg;
    return;
}
void CGroup3Detect::setAlgoParamCfg(const Alarm::GarbageExposureDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置垃圾暴露检测参数");
    m_stAlgoGarbageExposureCfg = stAlgoCfg;
}

void CGroup3Detect::setAlgoParamCfg(const Alarm::GarbageOverflowDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置垃圾满溢检测参数");
    m_stAlgoGarbageOverflowCfg = stAlgoCfg;
    return;
}

void CGroup3Detect::setAlgoParamCfg(const Alarm::ManholeCoverAbnormalDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置井盖异常检测参数");
    m_stAlgoManholeCoverAbnormalCfg = stAlgoCfg;
}

void CGroup3Detect::setAlgoParamCfg(const Alarm::RoadPondingDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置道路积水检测参数");
    m_stAlgoRoadPondingCfg = stAlgoCfg;
    return;
}

void CGroup3Detect::run()
{
    MediaData_S                            stMediaData;
    std::vector<Group3Detect_NS::Result_S> vecResult;

    while (m_bRunning.load())
    {
        if (!m_pHandle)
        {
            if (!init())
            {
                dlog_error("等待 模型组合3识别初始化");
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

        CStatisticsTimer runTime(" 模型组合3识别完整耗时");

        /* 送分析 */
        if (stMediaData.pData)
        {
            frameRate(" 模型组合3识别-分析数据", 5);

            Group3Detect_NS::InData_S  stInData{};
            Group3Detect_NS::OutData_S stOutData;

            cv::Mat i420Mat(
                stMediaData.stMediaParam.nVideoHeight * 3 / 2,
                stMediaData.stMediaParam.nVideoWidth,
                CV_8UC1,
                stMediaData.pData.get());

            /* rgb格式转换 */
            cv::Mat rgbMat;
            cv::cvtColor(i420Mat, rgbMat, cv::COLOR_YUV2RGB_NV12);

            /* 分辨率大小转换 */
            cv::resize(
                rgbMat,
                stInData.inMat,
                cv::Size(m_nWidth, m_nHeight),
                0,
                0,
                cv::INTER_LINEAR);

            // cv::rotate(stInData.inMat, stInData.inMat, cv::ROTATE_180);

            if (!stInData.inMat.empty())
            {
                if (access("group3Detect_debugImage", F_OK) == 0)
                {
                    dlog_debug("============>debugImage");
                    cv::imwrite("/opt/algo/Image/group3Detect_debugImage.jpg", stInData.inMat);
                }

                /* 烟雾 识别 */
                if (m_stAlgoSmokeFireCfg.bEnable)
                {
                    stInData.stParam.stSmokeFireDetectParam.bEnable      = true;
                    stInData.stParam.stSmokeFireDetectParam.fConfidence  = sensitivityToConfidence(m_stAlgoSmokeFireCfg.stRule.nSensitivity);
                    stInData.stParam.stSmokeFireDetectParam.nDetectFrame = sensitivityToFrames(m_stAlgoSmokeFireCfg.stRule.nSensitivity);
                }

                /* 火焰 识别 */
                if (m_stAlgoOpenFlameCfg.bEnable)
                {
                    stInData.stParam.stOpenFlameParam.bEnable      = true;
                    stInData.stParam.stOpenFlameParam.fConfidence  = sensitivityToConfidence(m_stAlgoOpenFlameCfg.stRule.nSensitivity);
                    stInData.stParam.stOpenFlameParam.nDetectFrame = sensitivityToFrames(m_stAlgoOpenFlameCfg.stRule.nSensitivity);
                }

                /* 垃圾暴露 识别 */
                if (m_stAlgoGarbageExposureCfg.bEnable)
                {
                    stInData.stParam.stGarbageExposureParam.bEnable      = true;
                    stInData.stParam.stGarbageExposureParam.fConfidence  = sensitivityToConfidence(m_stAlgoGarbageExposureCfg.stRule.nSensitivity);
                    stInData.stParam.stGarbageExposureParam.nDetectFrame = sensitivityToFrames(m_stAlgoGarbageExposureCfg.stRule.nSensitivity);
                }

                /* 垃圾满溢 识别 */
                if (m_stAlgoGarbageOverflowCfg.bEnable)
                {
                    stInData.stParam.stGarbageOverParam.bEnable      = true;
                    stInData.stParam.stGarbageOverParam.fConfidence  = sensitivityToConfidence(m_stAlgoGarbageOverflowCfg.stRule.nSensitivity);
                    stInData.stParam.stGarbageOverParam.nDetectFrame = sensitivityToFrames(m_stAlgoGarbageOverflowCfg.stRule.nSensitivity);
                }

                /* 井盖异常 识别 */
                if (m_stAlgoManholeCoverAbnormalCfg.bEnable)
                {
                    stInData.stParam.stManholeCoverAbnormalParam.bEnable      = true;
                    stInData.stParam.stManholeCoverAbnormalParam.fConfidence  = sensitivityToConfidence(m_stAlgoManholeCoverAbnormalCfg.stRule.nSensitivity);
                    stInData.stParam.stManholeCoverAbnormalParam.nDetectFrame = sensitivityToFrames(m_stAlgoManholeCoverAbnormalCfg.stRule.nSensitivity);
                    // dlog_debug(" ======= [井盖异常] %d %f %d================= ", stInData.stParam.stManholeCoverAbnormalParam.nDetectFrame, stInData.stParam.stManholeCoverAbnormalParam.fConfidence, m_stAlgoManholeCoverAbnormalCfg.stRule.nSensitivity);
                }

                /* 道路积水 识别 */
                if (m_stAlgoRoadPondingCfg.bEnable)
                {
                    stInData.stParam.stRoadPondingParam.bEnable      = true;
                    stInData.stParam.stRoadPondingParam.fConfidence  = sensitivityToConfidence(m_stAlgoRoadPondingCfg.stRule.nSensitivity);
                    stInData.stParam.stRoadPondingParam.nDetectFrame = sensitivityToFrames(m_stAlgoRoadPondingCfg.stRule.nSensitivity);
                }

                /* 分析数据 */
                {
                    CStatisticsTimer runTime(" 模型组合3识别算法耗时");
                    m_pHandle->process(stInData, vecResult, &stOutData);
                    /* 检测后处理 */
                    processGroup3Detect(stOutData);
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
            dlog_error("ai_app:  模型组合3识别-获取虚拟地址失败");
        }
    }
}

int CGroup3Detect::dynamicAnalysis(const std::vector<Group3Detect_NS::Result_S> &vecResult)
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
    if(vstRectInfo.size())
    {
        send_detectionResult_to_osd(m_nWidth, m_nHeight, vstRectInfo);
    }
    return 0;
}

void CGroup3Detect::processGroup3Detect(const Group3Detect_NS::OutData_S &stOutData)
{
    if (m_stAlgoSmokeFireCfg.bEnable)
    {
        m_SmokeFireStateMachine.handleAlarmState(stOutData.bSmoke, Event::Type_E::SMOKE_FIRE);
    }
    if (m_stAlgoOpenFlameCfg.bEnable)
    {
        m_OpenFlameStateMachine.handleAlarmState(stOutData.bOpenFire, Event::Type_E::OPEN_FLAME);
    }
    if (m_stAlgoGarbageExposureCfg.bEnable)
    {
        m_GarbageExposureStateMachine.handleAlarmState(stOutData.bGarbageExposure, Event::Type_E::GARBAGE_EXPOSURE);
    }
    if (m_stAlgoGarbageOverflowCfg.bEnable)
    {
        m_GarbageOverStateMachine.handleAlarmState(stOutData.bGarbageOver, Event::Type_E::GARBAGE_OVERFLOW);
    }
    if (m_stAlgoManholeCoverAbnormalCfg.bEnable)
    {
        m_ManholeCoverAbnormalStateMachine.handleAlarmState(stOutData.bManholeCoverAbnormal, Event::Type_E::MANHOLE_COVER_ABNORMAL);
    }
    if (m_stAlgoRoadPondingCfg.bEnable)
    {
        m_RoadPondingStateMachine.handleAlarmState(stOutData.bRoadPonding, Event::Type_E::ROAD_PONDING);
    }

    return;
}
