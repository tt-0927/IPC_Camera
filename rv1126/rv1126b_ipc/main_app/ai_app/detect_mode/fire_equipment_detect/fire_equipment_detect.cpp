/**
 * @file fire_equipment_detect.cpp
 * @author xushou (xushou@kfb.cn)
 * @date 2025-11-12
 *
 * @brief 消防设施异常检测
 */
#include "fire_equipment_detect.hpp"
#include "algo_stream_deal.h"
#include "StatisticsTimer.hpp"
#include "SaveImage.hpp"

/* 数据队列 */
#define QUEUE_MAX (2)

CFireEquipmentDetect::CFireEquipmentDetect()
    : m_dateQueue(QUEUE_MAX)
{
    m_bRunning.store(true);
    m_thread = std::thread(&CFireEquipmentDetect::run, this);
}

CFireEquipmentDetect::~CFireEquipmentDetect()
{
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

static void FireEquipmentPrintArea(const std::vector<::Event::Point_S>& area)
{
    std::cout << "当前区域包含 " << area.size() << " 个点：" << std::endl;
    for (size_t pointIdx = 0; pointIdx < area.size(); ++pointIdx) {
        const ::Event::Point_S& point = area[pointIdx];
        std::cout << "  点 " << pointIdx << "：(nX=" << point.nX << ", nY=" << point.nY << ")" << std::endl;
    }
    std::cout << "-------------------------" << std::endl;
}

void CFireEquipmentDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_stAlgoFireEquipmentCfg.bEnable)
    {
        dlog_debug("ai_app: 消防设施异常检测-开关未启用");
        return;
    }
    m_nChannelId = stMediaData.stMediaParam.nChannel;
    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("消防设施异常检测-数据队列满了 [%d]", m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

void CFireEquipmentDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_stAlgoFireEquipmentCfg.bEnable = stAlgoConfig.nEnFireEquipmentAbnormal;

    if (m_stAlgoFireEquipmentCfg.bEnable)
    {
        m_bRuleValid = false;
        m_stRuleInfo = Event::RuleInfo();
        m_bEventTriggered = false;
        m_highS1Rate = 0.0f;
        m_lastHighS1Time = {};
        m_highSustainedStart = {};
        m_freezeBgUntil = {};
        m_lastEndTime = {};
        m_eventTriggerStart = {};
        m_lastLowTime = {};

        Alarm::FireEquipmentAbnormalDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
}

void CFireEquipmentDetect::setAlgoParamCfg(const Alarm::FireEquipmentAbnormalDetection_S &stAlgoCfg,
                                            Event::Type_E enType)
{
    dlog_debug("ai_app: 设置消防设施异常检测参数");
    m_stAlgoFireEquipmentCfg = stAlgoCfg;
    m_lastBgUpdateTime -= std::chrono::seconds(10);

    m_fSensiThrd = 13 - m_stAlgoFireEquipmentCfg.stRule.nSensitivity / 10;

    convertResolutionAndEnable(m_stAlgoFireEquipmentCfg, Event::Type::FIRE_EQUIPMENT_ABNORMAL);
}

void CFireEquipmentDetect::drawRulesToImage(cv::Mat& inMat)
{
    if (!m_bRuleValid)
    {
        return;
    }
    for (const auto& area : m_stRuleInfo.areas)
    {
        std::vector<cv::Point> polygon;
        for (const auto& point : area)
        {
            polygon.emplace_back(point.nX, point.nY);
        }
        cv::polylines(inMat,
                      polygon,
                      true,
                      cv::Scalar(0, 255, 0),
                      2,
                      cv::LINE_AA);
    }
}

bool CFireEquipmentDetect::init()
{
    dlog_info("消防设施异常检测算法初始化成功");
    return true;
}

bool CFireEquipmentDetect::unInit()
{
    return true;
}

void CFireEquipmentDetect::run()
{
    MediaData_S stMediaData;

    std::chrono::steady_clock::time_point lastStayTime{};
    bool hasLastStayTime = false;

    cv::Mat currentFrame;
    cv::Mat grayPrev, grayCurrent;
    cv::Mat diff, mask, maskedDiff;
    std::vector<cv::Point> polygon;

    while (m_bRunning.load())
    {
        m_dateQueue.pop(stMediaData, -1);
        if (!m_bRuleValid || stMediaData.nSize == 0)
        {
            continue;
        }

        CStatisticsTimer runTime("消防设施异常检测完整耗时");

        cv::Mat i420Mat(
            stMediaData.stMediaParam.nVideoHeight * 3 / 2,
            stMediaData.stMediaParam.nVideoWidth,
            CV_8UC1,
            stMediaData.pData.get());

        cv::Mat rgbMat;
        cv::cvtColor(i420Mat, rgbMat, cv::COLOR_YUV2BGR_I420);
        m_lastRgbFrame = rgbMat.clone();

        if (1)
        {
            frameRate("消防设施异常检测-分析数据", 5);

            currentFrame = rgbMat.clone();

            if (!currentFrame.empty())
            {
                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    if (m_backgroundFrame.empty())
                    {
                        m_backgroundFrame = currentFrame.clone(); // 保存背景帧（第一次拍的画面）
                        continue;
                    }

                    /*转灰度图*/
                    cv::cvtColor(m_backgroundFrame, grayPrev, cv::COLOR_BGR2GRAY);
                    cv::cvtColor(currentFrame, grayCurrent, cv::COLOR_BGR2GRAY);

                    cv::absdiff(grayPrev, grayCurrent, diff);//计算两帧的差异，在diff 里：没变化的地方是黑色，变化的地方是白色

                    cv::threshold(diff, diff, 30, 255, cv::THRESH_BINARY);//二值化

                    cv::GaussianBlur(diff, diff, cv::Size(3, 3), 0.5);//高斯模糊

                    /*计算全图变化总和*/
                    double ST = cv::sum(diff)[0];

                    auto now = std::chrono::steady_clock::now();
                    auto timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastBgUpdateTime).count();

                    bool freezeBg = false;
                    if (m_stRuleInfo.enType == Event::Type::FIRE_EQUIPMENT_ABNORMAL && m_stAlgoFireEquipmentCfg.bEnable)
                    {
                        if (m_freezeBgUntil != std::chrono::steady_clock::time_point{} && now < m_freezeBgUntil)
                        {
                            freezeBg = true;
                        }
                    }

                    if (timeSinceLastUpdate > 10 && !freezeBg)
                    {
                        m_backgroundFrame = currentFrame.clone();
                        m_lastBgUpdateTime = now;
                        if (access("/testPrint", F_OK) == 0) { dlog_debug("ai_app: 消防设施异常检测: 背景帧已更新"); }
                        continue;
                    }

                    {
                        double stRate = ST / (diff.rows * diff.cols * 255 + 1e-5); //检查全局场景变化（摄像头晃动/光线突变）
                        if (stRate > 0.30)
                        {
                            m_backgroundFrame = currentFrame.clone();// 更新背景
                            m_lastBgUpdateTime = now;
                            m_highSustainedStart = {};
                            m_lastHighS1Time = {};
                            m_freezeBgUntil = {};
                            m_highS1Rate = 0.0f;
                            m_bEventTriggered = false;
                            m_lastEndTime = {};
                            m_eventTriggerStart = {};
                            m_lastLowTime = {};
                            hasLastStayTime = false;
                            if (access("/testPrint", F_OK) == 0) { dlog_debug("ai_app: 消防设施异常检测: 全局场景变化,更新背景"); }
                            continue;
                        }
                    }

                    for (const auto& area : m_stRuleInfo.areas)
                    {
                        if (access("/testPrint", F_OK) == 0)
                        {
                            dlog_debug("==============打印当前区域=================");
                            FireEquipmentPrintArea(area);
                            drawRulesToImage(currentFrame);
                            Modules_NS::saveImage(currentFrame, "/mnt/fire_equipment_test");
                        }

                        polygon.clear();
                        for (const auto& point : area)
                        {
                            polygon.emplace_back(point.nX, point.nY);
                        }

                        if (mask.size() != diff.size() || mask.type() != CV_8U)
                        {
                            mask.create(diff.size(), CV_8U);
                        }
                        mask.setTo(0);
                        cv::fillPoly(mask, polygon, cv::Scalar(255));

                        cv::bitwise_and(diff, diff, maskedDiff, mask);

                        double S1 = cv::sum(maskedDiff)[0];

                        double ST_rate = ST / (diff.rows * diff.cols * 255 + 1e-5);
                        double S1_rate = S1 / (cv::countNonZero(mask) * 255 + 1e-5);

                        float fObjDiffThrd = (ST_rate > 0.001) ? static_cast<float>((S1_rate / (ST_rate + 1e-5))) : 0.0f;
                        float areaRatio = cv::countNonZero(mask) / (float)(diff.rows * diff.cols);

                        if (m_stRuleInfo.enType == Event::Type::FIRE_EQUIPMENT_ABNORMAL && m_stAlgoFireEquipmentCfg.bEnable)
                        {
                            bool bTrigger;
                            if (areaRatio < 0.5f) {
                                bTrigger = (S1_rate * 100 > 4) && (fObjDiffThrd >= 0.45f) && (S1_rate - ST_rate > 0.01);
                            } else {
                                bTrigger = (S1_rate * 100 > 4) && (fObjDiffThrd >= 0.45f) && (ST_rate < 0.25);
                            }

                            if (m_lastEndTime != std::chrono::steady_clock::time_point{})
                            {
                                auto elapsedSinceEnd = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastEndTime).count();
                                if (elapsedSinceEnd < 5)
                                {
                                    if (m_lastLowTime != std::chrono::steady_clock::time_point{} && bTrigger)
                                    {
                                        auto lowDuration = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastLowTime).count();
                                        if (lowDuration >= 5)
                                        {
                                            dlog_debug("冷却期内检测到新变化(低电平>5s), 提前结束冷却");
                                            m_lastEndTime = {};
                                            hasLastStayTime = false;
                                            m_lastLowTime = {};
                                        }
                                        else
                                        {
                                            continue;
                                        }
                                    }
                                    else
                                    {
                                        continue;
                                    }
                                }
                                else
                                {
                                    dlog_debug("冷却期结束");
                                    hasLastStayTime = false;
                                    m_lastEndTime = {};
                                }
                            }

                            if (bTrigger)
                            {
                                m_lastBgUpdateTime = now;

                                if (!hasLastStayTime)
                                {
                                    dlog_debug("消防设施异常首次检测到变化");
                                    lastStayTime = now;
                                    hasLastStayTime = true;
                                    m_lastLowTime = {};
                                }
                                else if (!m_bEventTriggered)
                                {
                                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastStayTime).count();
                                    if (duration >= m_nLeaveTimeThrd)
                                    {
                                        m_bEventTriggered = true;
                                        m_eventTriggerStart = now;
                                        dlog_debug("=======ai_app: 消防设施异常事件触发==========");
#ifdef ENABLE_TVSDK_SRC
                                        {
                                            EventTriggerContext_S stCtx;
                                            stCtx.enEventType = Event::Type::FIRE_EQUIPMENT_ABNORMAL;
                                            stCtx.nChnId = m_nChannelId;
                                            stCtx.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                                std::chrono::system_clock::now().time_since_epoch()).count();
                                            if (!m_lastRgbFrame.empty()) {
                                                auto pPayload = std::make_shared<EventTvSdkPayload_S>();
                                                pPayload->enType = get_tvsdk_payload_type(stCtx.enEventType);
                                                if (encode_mat_to_tvsdk_image(m_lastRgbFrame, pPayload->stPanoramaImage, JPEG_QUALITY_PANORAMA, false)) {
                                                    stCtx.pTvSdkPayload = pPayload;
                                                }
                                            }
                                            stCtx.bEventEnded = false;
                                            CEventLinkage::instance()->handleEvent(stCtx);
                                        }
#else
                                        CEventLinkage::instance()->handleEvent(Event::Type::FIRE_EQUIPMENT_ABNORMAL, false);
#endif
                                    }
                                }
                            }
                            else
                            {
                                if (hasLastStayTime)
                                {
                                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastStayTime).count();
                                    if (duration >= 1800)
                                    {
                                        dlog_debug("消防设施异常检测 - 变化消失超过1.8秒, 重置检测");
                                        m_lastLowTime = now;
                                        hasLastStayTime = false;
                                        if (m_bEventTriggered)
                                        {
                                            dlog_debug("=======ai_app: 消防设施异常事件结束==========");
#ifdef ENABLE_TVSDK_SRC
                                            {
                                                EventTriggerContext_S stCtx;
                                                stCtx.enEventType = Event::Type::FIRE_EQUIPMENT_ABNORMAL;
                                                stCtx.nChnId = m_nChannelId;
                                                stCtx.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                                    std::chrono::system_clock::now().time_since_epoch()).count();
                                                if (!m_lastRgbFrame.empty()) {
                                                    auto pPayload = std::make_shared<EventTvSdkPayload_S>();
                                                    pPayload->enType = get_tvsdk_payload_type(stCtx.enEventType);
                                                    if (encode_mat_to_tvsdk_image(m_lastRgbFrame, pPayload->stPanoramaImage, JPEG_QUALITY_PANORAMA, false)) {
                                                        stCtx.pTvSdkPayload = pPayload;
                                                    }
                                                }
                                                stCtx.bEventEnded = true;
                                                CEventLinkage::instance()->handleEvent(stCtx);
                                            }
#else
                                            CEventLinkage::instance()->handleEvent(Event::Type::FIRE_EQUIPMENT_ABNORMAL, true);
#endif
                                            m_bEventTriggered = false;
                                            m_eventTriggerStart = {};
                                            m_lastEndTime = now;
                                        }
                                    }
                                }
                            }

                            if (m_bEventTriggered)
                            {
                                if (m_eventTriggerStart != std::chrono::steady_clock::time_point{})
                                {
                                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_eventTriggerStart).count();
                                    if (elapsed > 2500)
                                    {
                                        dlog_debug("ai_app: 消防设施异常事件触发超时(%lldms), 强制结束", (long long)elapsed);
#ifdef ENABLE_TVSDK_SRC
                                        {
                                            EventTriggerContext_S stCtx;
                                            stCtx.enEventType = Event::Type::FIRE_EQUIPMENT_ABNORMAL;
                                            stCtx.nChnId = m_nChannelId;
                                            stCtx.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                                std::chrono::system_clock::now().time_since_epoch()).count();
                                            if (!m_lastRgbFrame.empty()) {
                                                auto pPayload = std::make_shared<EventTvSdkPayload_S>();
                                                pPayload->enType = get_tvsdk_payload_type(stCtx.enEventType);
                                                if (encode_mat_to_tvsdk_image(m_lastRgbFrame, pPayload->stPanoramaImage, JPEG_QUALITY_PANORAMA, false)) {
                                                    stCtx.pTvSdkPayload = pPayload;
                                                }
                                            }
                                            stCtx.bEventEnded = true;
                                            CEventLinkage::instance()->handleEvent(stCtx);
                                        }
#else
                                        CEventLinkage::instance()->handleEvent(Event::Type::FIRE_EQUIPMENT_ABNORMAL, true);
#endif
                                        m_bEventTriggered = false;
                                        hasLastStayTime = false;
                                        m_lastEndTime = now;
                                        m_eventTriggerStart = {};
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                dlog_error("ai_app: 图片数据为空");
            }
        }
        else
        {
            dlog_error("ai_app: 消防设施异常检测-获取虚拟地址失败");
        }
    }
}

template<typename T>
void CFireEquipmentDetect::convertResolutionAndEnable(T &stConfig, Event::Type enType)
{
    m_bRuleValid = false;
    m_stRuleInfo = Event::RuleInfo();

    auto &rule = stConfig.stRule;
    Event::RuleInfo stRule;
    std::vector<::Event::Point_S> area;

    if (!rule.stRegion.IsValid())
    {
        dlog_debug("当前多边形区域无效");
        stConfig.bEnable = false;
        return;
    }

    bool bVaildPoint = false;
    for (int i = 0; i < 4; ++i)
    {
        const auto& point = rule.stRegion.aPoint[i];
        if (point.fX != 0 || point.fY != 0)
        {
            bVaildPoint = true;
            dlog_debug("当前多边形区域有效");
            break;
        }
    }
    if (!bVaildPoint)
    {
        dlog_debug("当前多边形区域无效");
        stConfig.bEnable = false;
        return;
    }

    stRule.bEnable = true;
    rule.stRegion.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight);

    stRule.enType = enType;
    stRule.nSensitivity = rule.nSensitivity;
    dlog_debug("ai_app:  当前规则获取到 灵敏度[%d] ", rule.nSensitivity);
    for (auto &pos : rule.stRegion.aPoint)
    {
        ::Event::Point_S stPoint;
        stPoint.nX = pos.fX;
        stPoint.nY = pos.fY;
        area.push_back(stPoint);
    }
    stRule.areas.push_back(area);

    m_stRuleInfo = stRule;
    m_bRuleValid = true;
    stConfig.bEnable = true;
}
