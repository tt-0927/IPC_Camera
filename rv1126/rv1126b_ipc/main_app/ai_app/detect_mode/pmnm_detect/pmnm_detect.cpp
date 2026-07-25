/**
 * @file pmnm_detect.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-05
 * 
 * @brief 机动车、行人、非机动车检测相关
 */

#include "pmnm_detect.hpp"
#include "common_process.h"
#include "StatisticsTimer.hpp"
#ifdef ENABLE_GAT1400_SRC
#include "gat1400.h"
#endif

/* 数据队列 */
#define QUEUE_MAX (2)
//人体检测目标
#define PERSON_DETECT_TARGET    0 

CPMNMDetect::CPMNMDetect()
 : m_dateQueue(QUEUE_MAX)
{
	m_bRunning.store(true);
    m_thread = std::thread(&CPMNMDetect::run, this);
}

CPMNMDetect::~CPMNMDetect()
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

static void printArea(const std::vector<::Event::Point_S>& area) 
{
    // 打印区域包含的点数量
    std::cout << "当前区域包含 " << area.size() << " 个点：" << std::endl;
    
    // 遍历区域内的每个点
    for (size_t pointIdx = 0; pointIdx < area.size(); ++pointIdx) {
        const ::Event::Point_S& point = area[pointIdx];  // 获取当前点（注意命名空间::Event::）
        // 打印点的索引和坐标（nX 为x坐标，nY 为y坐标）
        std::cout << "  点 " << pointIdx << "：(nX=" << point.nX << ", nY=" << point.nY << ")" << std::endl;
    }
    std::cout << "-------------------------" << std::endl;
}


void CPMNMDetect::recvMediaData(MediaData_S stMediaData)
{
	if (!m_stAlgoCrossCfg.bEnable && !m_stAlgoIntruCfg.bEnable && !m_stAlgoEntryCfg.bEnable &&!m_stAlgoExitCfg.bEnable &&
        !m_stAlgoEmergencyLaneOccupancyCfg.bEnable && !m_stAlgoNonMotorVehicleIntrusionCfg.bEnable)
    {
        dlog_debug("ai_app: 机动车、行人、非机动车检测-开关未启用");
        return;
    }

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("ai_app: 机动车、行人、非机动车检测-数据队列满了");
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

void CPMNMDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
	m_stAlgoCrossCfg.bEnable = stAlgoConfig.nEnLineCrossing;
    m_stAlgoIntruCfg.bEnable = stAlgoConfig.nEnIntrusion;
    m_stAlgoEntryCfg.bEnable = stAlgoConfig.nEnEnterRegion;
    m_stAlgoExitCfg.bEnable = stAlgoConfig.nEnLeaveRegion;
    m_stAlgoEmergencyLaneOccupancyCfg.bEnable = stAlgoConfig.nEnEmergencyLaneOccupancy;
    m_stAlgoNonMotorVehicleIntrusionCfg.bEnable = stAlgoConfig.nEnNonMotorVehicleIntrusion;
    
    if (m_stAlgoCrossCfg.bEnable)
    {
		Alarm::BoundaryDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
    if (m_stAlgoIntruCfg.bEnable)
    {
        Alarm::FieldDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
    if (m_stAlgoEntryCfg.bEnable)
    {
        Alarm::EntranceDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
    if (m_stAlgoExitCfg.bEnable)
    {
        Alarm::ExitingDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_stAlgoEmergencyLaneOccupancyCfg.bEnable)
    {
        Alarm::EmergencyLaneOccupancyDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_stAlgoNonMotorVehicleIntrusionCfg.bEnable)
    {
        Alarm::NonMotorVehicleIntrusionDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
        m_pHandle->resetNonMotorVehicleIntrusionStatus();

    }

}

void CPMNMDetect::setAlgoParamCfg(const Alarm::BoundaryDetection_S &stAlgoCfg,Event::Type_E enType)
{
	dlog_debug("ai_app:  设置越界检测参数");
    m_stAlgoCrossCfg = stAlgoCfg;
    convertBoundaryAndEnable(m_stAlgoCrossCfg);
}

void CPMNMDetect::setAlgoParamCfg(const Alarm::FieldDetection_S &stAlgoCfg,Event::Type_E enType)
{
	dlog_debug("ai_app:  设置区域入侵参数");
    m_stAlgoIntruCfg = stAlgoCfg;
    m_vstIntruRule.clear();
    convertResolutionAndEnable(m_stAlgoIntruCfg, enType); 
}

void CPMNMDetect::setAlgoParamCfg(const Alarm::EntranceDetection_S &stAlgoCfg,Event::Type_E enType)
{
	dlog_debug("ai_app:  设置进入区域参数");
    m_stAlgoEntryCfg = stAlgoCfg;
    m_vstEntryRule.clear();
    convertResolutionAndEnable(m_stAlgoEntryCfg, enType); 
}

void CPMNMDetect::setAlgoParamCfg(const Alarm::ExitingDetection_S &stAlgoCfg,Event::Type_E enType)
{
	dlog_debug("ai_app:  设置离开区域参数");
    m_stAlgoExitCfg = stAlgoCfg;
    m_vstExitRule.clear();
    convertResolutionAndEnable(m_stAlgoExitCfg, enType); 
}

void CPMNMDetect::setAlgoParamCfg(const Alarm::EmergencyLaneOccupancyDetection_S &stAlgoCfg,Event::Type_E enType)
{
	dlog_debug("ai_app:  设置应急车道占用参数");
    m_stAlgoEmergencyLaneOccupancyCfg = stAlgoCfg;
    m_vstEmergencyLaneOccupancyRule.clear();
    convertResolutionAndEnable(m_stAlgoEmergencyLaneOccupancyCfg, enType); 
}

void CPMNMDetect::setAlgoParamCfg(const Alarm::NonMotorVehicleIntrusionDetection_S &stAlgoCfg,Event::Type_E enType)
{
	dlog_debug("ai_app:  设置非机动车闯入参数");
    m_stAlgoNonMotorVehicleIntrusionCfg = stAlgoCfg;
    m_vstNonMotorVehicleIntrusionRule.clear();
    convertResolutionAndEnable(m_stAlgoNonMotorVehicleIntrusionCfg, enType); 
}

float CPMNMDetect::sensitivityToConfidence(int sensitivity, float minConfidence, float maxConfidence) 
{
    int clampedSens = std::clamp(sensitivity, 1, 100);

    float confidence = maxConfidence - (clampedSens - 1) * (maxConfidence - minConfidence) / (100 - 1);

    confidence = std::clamp(confidence, minConfidence, maxConfidence);

    return confidence;
}

int CPMNMDetect::sensitivityToFrames(int sensitivity, int minFrames, int maxFrames) 
{
    if (sensitivity <= 0) return maxFrames;
    if (sensitivity >= 100) return minFrames;
    
    // 线性映射
    double ratio = (100.0 - sensitivity) / 100.0;
    int frames = minFrames + static_cast<int>(ratio * (maxFrames - minFrames));
    
    return frames;
}

bool CPMNMDetect::init()
{
    if (!m_pHandle)
    {
        PMNMDetect_NS::InParam_S stInParam;
        stInParam.strModelPath = "/opt/cam/model/PersonMotorNomotorDetect.json";
        stInParam.bDebug = false;

        m_pHandle = new PMNMDetect_NS::CPMNMDetectV2_0(stInParam);
        if (m_pHandle)
        {
            if (m_pHandle->init())
            {
                dlog_debug("ai_app: 机动车、行人、非机动车检测算法初始化成功, %s", stInParam.strModelPath.c_str());
              
                return true;
            }
            else
            {
                delete m_pHandle;
                m_pHandle = nullptr;
                dlog_debug("边界检测算法初始化失败");
            }
        }
    }
    return false;
}
 
bool CPMNMDetect::unInit()
{
    if (m_pHandle)
    {
        delete m_pHandle;
        m_pHandle = nullptr;
    }
    
    return true;
}


void CPMNMDetect::changeRuleInfos(const Event::RuleInfo &stRuleInfo, std::vector<int> veDetectionTargets, bool bNeedClear)
{
    if (bNeedClear)
    {
        m_vstRuleInfo.clear();
    }
    
    if (stRuleInfo.enType == Event::Type::LINE_CROSSING || stRuleInfo.enType == Event::Type::INTRUSION || 
        stRuleInfo.enType == Event::Type::ENTER_REGION || stRuleInfo.enType == Event::Type::LEAVE_REGION || 
        stRuleInfo.enType == Event::Type_E::EMERGENCY_LANE_OCCUPANCY)
    {
        dlog_debug("ai_app:  更新边界检测划线数据 type[%d]", (int)stRuleInfo.enType);
        PMNMDetectRuleInfo_S stPMNMDetectRuleInfo;
        stPMNMDetectRuleInfo.stRuleInfo = stRuleInfo;
        stPMNMDetectRuleInfo.veDetectionTargets = veDetectionTargets;

        m_vstRuleInfo.push_back(stPMNMDetectRuleInfo);
    }
}

void CPMNMDetect::drawRulesToImage(cv::Mat& inMat)
{
    for (const auto& rule : m_vstRuleInfo)
    {
        /* 绘制所有的线条 */
        for (const auto& line : rule.stRuleInfo.lines)
        {
            if (line.size() < 2)
            {
                continue;
            }
            
            for (size_t i = 0; i < line.size() - 1; ++i)
            {
                cv::line(inMat, 
                         cv::Point(line[i].nX, line[i].nY),
                         cv::Point(line[i + 1].nX, line[i + 1].nY),
                         cv::Scalar(0, 0, 255), /* 线条颜色（蓝色） */
                         2);
            }
        }

        /* 绘制所有的区域（四边形边框） */
        for (const auto& area : rule.stRuleInfo.areas)
        {
            std::vector<cv::Point> polygon;
            for (const auto& point : area)
            {
                polygon.emplace_back(point.nX, point.nY);
            }
            cv::polylines(inMat, 
                          polygon, 
                          true,
                          cv::Scalar(0, 255, 0), /* 边框颜色（绿色） */
                          2,
                          cv::LINE_AA);
        }
    }
}

/* 事件转换函数 */
int CPMNMDetect::convertMaskToType( int eventFlags)
{
    // dlog_debug("ai_app: 边界检测事件 [%s]\n", std::bitset<4>(eventFlags).to_string().c_str());
    
    { if (eventFlags & 0x01) return 8; /* Event::Type::LINE_CROSSING */ }
    { if (eventFlags & 0x02) return 9; /* Event::Type::INTRUSION */ }
    { if (eventFlags & 0x04) return 10; /* Event::Type::ENTER_REGION */ }
    { if (eventFlags & 0x08) return 11; /* Event::Type::LEAVE_REGION */ }
    { if (eventFlags & 0x10) return 44; /* Event::Type::EMERGENCY_LANE_OCCUPANCY */ }
    { if (eventFlags & 0x20) return 46; /* Event::Type::NON_MOTOR_VEHICLE_INTRUSION */ }

    return -1;
}

void CPMNMDetect::run()
{
    MediaData_S      stMediaData;
    std::vector<int> vResultData;
    vResultData.clear();
    // VIDEO_FRAME_INFO_S* pSrcFrame = nullptr;
   

    std::vector<PMNMDetect_NS::Result_S> vecResult;

    while (m_bRunning.load())
    {
        if (!m_pHandle)
        {
            if (!init())
            {
                dlog_error("等待边界检测初始化");
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
      
         //dlog_debug("ai_app [机动车、行人、非机动车检测]: 处理前Image分辨率[ %d × %d ] 格式[%d] 大小[%d]",
        //  stMediaData.stMediaParam.nVideoWidth,stMediaData.stMediaParam.nVideoHeight
        //  ,stMediaData.stMediaParam.enPixelFormat,stMediaData.nSize);

        CStatisticsTimer runTime("边界检测完整耗时");

        /* 送分析 */
        if (1)
        {
            frameRate("边界检测-分析数据", 5);

            PMNMDetect_NS::InData_S stInData {};
            PMNMDetect_NS::OutData_S stOutData;
            
             cv::Mat i420Mat(
                stMediaData.stMediaParam.nVideoHeight * 3/2,
                stMediaData.stMediaParam.nVideoWidth,
                CV_8UC1,
                stMediaData.pData.get()
            );

            stOutData.nType = 0;
            stOutData.validResult = false;
            /* rgb格式转换 */
            cv::Mat rgbMat;
            cv::cvtColor(i420Mat, rgbMat, cv::COLOR_YUV2RGB_NV12);
            // cv::rotate(rgbMat, rgbMat, cv::ROTATE_180);

            /* 分辨率大小转换 */
            cv::resize(
                rgbMat,          
                stInData.inMat,              
                cv::Size(m_nWidth, m_nHeight),  
                0, 0,                    
                cv::INTER_LINEAR 
            );

            if (!stInData.inMat.empty())
            {
                if (access("/debugImage", F_OK) == 0)
                {
                    dlog_debug("============>debugImage");
                    cv::imwrite("/opt/algo/Image/test_algo.jpg", stInData.inMat);
                }
                stInData.stParam.bVecEnable = true;
                /* 越界规则 */
                if(m_stAlgoCrossCfg.bEnable)
                {
                    for (auto &CrossRule : m_vstCrossRule)
                    {
                        PMNMDetect_NS::TripLineParam_S  stTripLineParam;    
                        if (!CrossRule.stRuleInfo.lines.empty() && CrossRule.stRuleInfo.bEnable && CrossRule.veDetectionTargets.size() > 0)
                        {
                            stTripLineParam.bEnable = true;
                            for (const auto& line : CrossRule.stRuleInfo.lines)
                            {
                                if (line.size() >= 2)
                                {       
                                    stTripLineParam.alertLine1 = cv::Point(line[0].nX, line[0].nY);
                                    stTripLineParam.alertLine2 = cv::Point(line[1].nX, line[1].nY);
                                }
                            }
                        }
                        else
                        {
                            stTripLineParam.bEnable = false;
                        }
                        stTripLineParam.veDetectionTargetTypes = CrossRule.veDetectionTargets;
                        stInData.stParam.vstTripLineParam.push_back(stTripLineParam); 
                    }
                }

                /* 区域入侵规则 */
                if(m_stAlgoIntruCfg.bEnable)
                {
                    for (auto &IntruRule : m_vstIntruRule)
                    {
                        PMNMDetect_NS::IntrusionParam_S stIntrusionParam;    
                        if (!IntruRule.stRuleInfo.areas.empty() && IntruRule.stRuleInfo.bEnable && IntruRule.veDetectionTargets.size() > 0)
                        {
                            stIntrusionParam.bEnable = true;
                            for (const auto& point : IntruRule.stRuleInfo.areas[0])
                            {
                                stIntrusionParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }
                        }
                        else
                        {
                            stIntrusionParam.bEnable = false;
                        }
                        stIntrusionParam.veDetectionTargetTypes = IntruRule.veDetectionTargets;
                        stInData.stParam.vstIntrusionParam.push_back(stIntrusionParam); 
                    }
                }

                /* 进入区域规则 */
                if(m_stAlgoEntryCfg.bEnable)
                {
                    for (auto &EntryRule : m_vstEntryRule)
                    {
                        PMNMDetect_NS::EntryParam_S     stEntryParam;
                        if (!EntryRule.stRuleInfo.areas.empty() && EntryRule.stRuleInfo.bEnable && EntryRule.veDetectionTargets.size() > 0)
                        {
                            stEntryParam.bEnable = true;
                            for (const auto& point : EntryRule.stRuleInfo.areas[0])
                            {
                                stEntryParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }
                        }
                        else
                        {
                            stEntryParam.bEnable = false;
                        }
                        stEntryParam.veDetectionTargetTypes = EntryRule.veDetectionTargets;
                        stInData.stParam.vstEntryParam.push_back(stEntryParam); 
                    }
                }

                 /* 离开区域规则 */
                if(m_stAlgoExitCfg.bEnable)
                {
                    for (auto &ExitRule : m_vstExitRule)
                    {
                        PMNMDetect_NS::LeaveParam_S     stLeaveParam;
                        if (!ExitRule.stRuleInfo.areas.empty() && ExitRule.stRuleInfo.bEnable && ExitRule.veDetectionTargets.size() > 0)
                        {
                            stLeaveParam.bEnable = true;
                            for (const auto& point : ExitRule.stRuleInfo.areas[0])
                            {
                                stLeaveParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }
                        }
                        else
                        {
                            stLeaveParam.bEnable = false;
                        }
                        
                        stLeaveParam.veDetectionTargetTypes = ExitRule.veDetectionTargets;
                        stInData.stParam.vstLeaveParam.push_back(stLeaveParam); 
                    }
                }

                /* 应急车道规则 */
                if(m_stAlgoEmergencyLaneOccupancyCfg.bEnable)
                {
                    for (auto &EmergencyLaneOccupancy : m_vstEmergencyLaneOccupancyRule)
                    {
                        PMNMDetect_NS::EmergencyLaneOccupancyParam_S     stEmergencyLaneOccupancyParam;
                        if (!EmergencyLaneOccupancy.stRuleInfo.areas.empty() && EmergencyLaneOccupancy.stRuleInfo.bEnable && EmergencyLaneOccupancy.veDetectionTargets.size() > 0)
                        {
                            stEmergencyLaneOccupancyParam.bEnable = true;
                            for (const auto& point : EmergencyLaneOccupancy.stRuleInfo.areas[0])
                            {
                                stEmergencyLaneOccupancyParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }
                        }
                        else
                        {
                            stEmergencyLaneOccupancyParam.bEnable = false;
                        }
                        
                        stEmergencyLaneOccupancyParam.fEmergencyLaneOccupancyThreshold = sensitivityToConfidence(EmergencyLaneOccupancy.stRuleInfo.nSensitivity);
                        stEmergencyLaneOccupancyParam.nEmergencyLaneOccupancyTimeThreshold = EmergencyLaneOccupancy.stRuleInfo.nTimeThreshold * 1000; // s转换为ms
                        stEmergencyLaneOccupancyParam.veDetectionTargetTypes = EmergencyLaneOccupancy.veDetectionTargets;
                        stInData.stParam.vstEmergencyLaneOccupancyParam.push_back(stEmergencyLaneOccupancyParam); 
                    }
                }

                /* 非机动车闯入规则 */
                if(m_stAlgoNonMotorVehicleIntrusionCfg.bEnable)
                {
                    for (auto &NonMotorVehicleIntrusionRule : m_vstNonMotorVehicleIntrusionRule)
                    {
                        PMNMDetect_NS::NonMotorVehicleIntrusionParam_S     stNonMotorVehicleIntrusionParam;
                        if (!NonMotorVehicleIntrusionRule.stRuleInfo.areas.empty() && NonMotorVehicleIntrusionRule.stRuleInfo.bEnable /*&& NonMotorVehicleIntrusionRule.veDetectionTargets.size() > 0*/)
                        {
                            stNonMotorVehicleIntrusionParam.bEnable = true;
                            for (const auto& point : NonMotorVehicleIntrusionRule.stRuleInfo.areas[0])
                            {
                                stNonMotorVehicleIntrusionParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }
                        }
                        else
                        {
                            stNonMotorVehicleIntrusionParam.bEnable = false;
                        }
                        stNonMotorVehicleIntrusionParam.fNonMotorVehicleIntrusionThreshold = sensitivityToConfidence(NonMotorVehicleIntrusionRule.stRuleInfo.nSensitivity);
                        stNonMotorVehicleIntrusionParam.nNonMotorVehicleIntrusionTimeThreshold = NonMotorVehicleIntrusionRule.stRuleInfo.nTimeThreshold * 1000; // s转换为ms
                        // stNonMotorVehicleIntrusionParam.veDetectionTargetTypes = NonMotorVehicleIntrusionRule.veDetectionTargets;
                        stInData.stParam.vstNonMotorVehicleIntrusionParam.push_back(stNonMotorVehicleIntrusionParam);
                    }
                }

                /* 分析数据 */
                {
                    CStatisticsTimer runTime("边界检测算法耗时");
                    m_pHandle->process(stInData, vecResult, &stOutData);

#if 0
                    std::vector<PMNMDetect_NS::Result_S> vecResult1;
                    std::vector<Common::RectInfo_S> vstRectInfo;
                    m_pHandle->process(stInData, vecResult, vecResult1, &stOutData);
                    for(unsigned int i = 0; i < vecResult1.size(); i++)
                    {
                        Common::RectInfo_S stRectInfo;
                        stRectInfo.nX1 = (int)vecResult1.at(i).fX1;
                        stRectInfo.nY1 = (int)vecResult1.at(i).fY1;
                        stRectInfo.nX2 = (int)(vecResult1.at(i).fX2);
                        stRectInfo.nY2 = (int)(vecResult1.at(i).fY2);
                        vstRectInfo.push_back(stRectInfo);
                    }

                    if(vstRectInfo.size() > 0)
                    {
                        send_detectionResult_to_osd(m_nWidth, m_nHeight, vstRectInfo);
                    }
#endif      
                    if (stOutData.validResult)
                    {
                        for (const auto& result : vecResult)
                        {
                            int nType = convertMaskToType(stOutData.nType);
                            if (nType != -1 /*&& m_EventManager.handleEvent(0,  stOutData.nType)*/ )
                            {
                                /* 上报事件 */
                                dlog_debug("ai_app: 机动车、行人、非机动车边界检测报警触发 类型[%d]",nType);
                                CEventLinkage::instance()->handleEvent(static_cast<Event::Type_E>(nType), false);
                            }
                        }
#ifdef ENABLE_GAT1400_SRC
                        Network::Gat1400Client_S config;
                        GAT1400::CGAT1400::instance()->getGat1400Config(config);
                        if (config.enableGat1400) {
                            cv::Mat imageMat;
                            cv::cvtColor(i420Mat, imageMat, cv::COLOR_YUV2RGB_NV12);
                            pushImageToGat1400(imageMat, vecResult);
                        }
#endif
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
            dlog_error("ai_app: 边界检测-获取虚拟地址失败");
        }
    }
}

void CPMNMDetect::convertBoundaryAndEnable(Alarm::BoundaryDetection_S &stConfig)
{
    if (!stConfig.aRule.empty())
    {
        m_vstCrossRule.clear();
        /* 转换警戒线坐标分辨率至算法分辨率 */
        for (auto &rule : stConfig.aRule)
        {
            bool bIsInit = false;
            Event::RuleInfo stRule;
            /* 转换起始点坐标 */
            float scaleX = static_cast<float>(m_nWidth) / PIXEL_WIDTH_1920;
            float scaleY = static_cast<float>(m_nHeight) / PIXEL_HEIGHT_1080;
            
            rule.stStartPos.fX *= scaleX;
            rule.stStartPos.fY *= scaleY;
            rule.stEndPos.fX *= scaleX;
            rule.stEndPos.fY *= scaleY;
            
            /* 判断是否设置了有效的警戒线 */
            if (((rule.stStartPos.fX != rule.stEndPos.fX)  || 
                 (rule.stStartPos.fY != rule.stEndPos.fY)) && 
                (rule.stStartPos.fX != 0 && rule.stStartPos.fY != 0 && rule.stEndPos.fX != 0 && rule.stEndPos.fY != 0))
            {
                bIsInit = true; 
            }
        
            /* 没有一条有效的警戒线，不使能 */
            if (!bIsInit)
            {
                dlog_debug("ai_app: 边界检测-越界检测警戒线无效");
                stRule.bEnable = false;
            }
            else
            {
                dlog_debug("ai_app: 边界检测-越界检测警戒线有效");
                std::vector<::Event::Point_S> line;
                ::Event::Point_S stPoint;
                stPoint.nX = rule.stStartPos.fX;
                stPoint.nY = rule.stStartPos.fY;
                line.push_back(stPoint);
                stPoint.nX = rule.stEndPos.fX;
                stPoint.nY = rule.stEndPos.fY;
                line.push_back(stPoint);
                // /* 判断是否开启了人体检测 */
                // bool bHasPerson = std::find(rule.aDetectionTarget.begin(), rule.aDetectionTarget.end(), PERSON_DETECT_TARGET) != rule.aDetectionTarget.end();
                // if(!bHasPerson)
                // {
                //     stRule.bEnable = false;
                //     dlog_debug("ai_app:  当前越界检测规则未开启人体检测");
                //     continue;
                // }
                // else
                {
                    stRule.bEnable = true;
                    stRule.lines.push_back(line);
                }
            }
            if(stRule.bEnable)
            {
                PMNMDetectRuleInfo_S stPMNMDetectRuleInfo;

                stPMNMDetectRuleInfo.stRuleInfo = stRule;
                stPMNMDetectRuleInfo.veDetectionTargets = rule.aDetectionTarget;
                m_vstCrossRule.push_back(stPMNMDetectRuleInfo);
            }
        }
    }
}

template<typename T>
void CPMNMDetect::convertResolutionAndEnable(T &stConfig,Event::Type_E enType)
{
    if (!stConfig.aRule.empty())
    {
        /* 是否有任何一个区域初始化成功 */
        // bool bIsInit = false;
        
        /* 转换区域坐标分辨率至算法分辨率 */
        for (auto &rule : stConfig.aRule) /* 使用引用而不是值拷贝 */
        {
            Event::RuleInfo stRule;
            std::vector<::Event::Point_S> area;
             /* 判断是否设置了正确的多边形 */
            if (!rule.stRegion.IsValid())
            {
                stRule.bEnable = false;
                dlog_debug("当前多边形区域无效");
                continue;
            }

            bool bVaildPoint = false;
            for (int i = 0; i < 4; ++i) 
            {
                const auto& point = rule.stRegion.aPoint[i];

                if (point.fX != 0 || point.fY != 0 ) 
                {
                    bVaildPoint = true;
                    dlog_debug("当前多边形区域有效");
                    break;
                }
            }
            if(!bVaildPoint)
            {
                stRule.bEnable = false;
                dlog_debug("当前多边形区域无效");
                continue;
            }

            rule.stRegion.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight);
            // bool bHasPerson = true;
            // if(enType == Event::Type_E::INTRUSION || enType == Event::Type_E::ENTER_REGION || 
            //    enType == Event::Type_E::LEAVE_REGION || enType == Event::Type_E::EMERGENCY_LANE_OCCUPANCY)
            // {
            //     bHasPerson = std::find(rule.aDetectionTarget.begin(), rule.aDetectionTarget.end(), PERSON_DETECT_TARGET) != rule.aDetectionTarget.end();
            // }

            // if(!bHasPerson)
            // {
            //     stRule.bEnable = false;
            //     dlog_debug("ai_app:  当前规则未开启人体检测");
            // }
            // else
            {
                stRule.bEnable = true;
                stRule.nSensitivity = rule.nSensitivity;
                stRule.nTimeThreshold = rule.nTimeThreshold;
                dlog_debug("ai_app:  当前规则获取到 灵敏度[%d] 时间阈值[%d] ",rule.nSensitivity,rule.nTimeThreshold);
                for (auto &pos : rule.stRegion.aPoint)
                {
                    ::Event::Point_S stPoint;
                    stPoint.nX = pos.fX;
                    stPoint.nY = pos.fY;
                    area.push_back(stPoint);
                }
                stRule.areas.push_back(area);
            }
                
            if(!stRule.bEnable)
            {
                dlog_debug("当前规则无效");
                continue;
            }
            switch (enType)
            {
                case Event::Type_E::INTRUSION:
                {
                    PMNMDetectRuleInfo_S stPMNMDetectRuleInfo;
                    stPMNMDetectRuleInfo.stRuleInfo = stRule;
                    stPMNMDetectRuleInfo.veDetectionTargets = rule.aDetectionTarget;
                    m_vstIntruRule.push_back(stPMNMDetectRuleInfo);
                #if HumanAreaDetect_DEBUG
                    dlog_debug("=====================打印区域入侵area=================");
                    printArea(area);
                #endif
                    break;
                }
                
                case Event::Type_E::ENTER_REGION:
                {
                    PMNMDetectRuleInfo_S stPMNMDetectRuleInfo;
                    stPMNMDetectRuleInfo.stRuleInfo = stRule;
                    stPMNMDetectRuleInfo.veDetectionTargets = rule.aDetectionTarget;
                    m_vstEntryRule.push_back(stPMNMDetectRuleInfo);
                #if HumanAreaDetect_DEBUG
                    dlog_debug("=====================打印进入区域area=================");
                    printArea(area);
                #endif
                    break;
                }
                
                case Event::Type_E::LEAVE_REGION:
                {
                    PMNMDetectRuleInfo_S stPMNMDetectRuleInfo;
                    stPMNMDetectRuleInfo.stRuleInfo = stRule;
                    stPMNMDetectRuleInfo.veDetectionTargets = rule.aDetectionTarget;
                    m_vstExitRule.push_back(stPMNMDetectRuleInfo);
                #if HumanAreaDetect_DEBUG
                    dlog_debug("=====================打印离开区域area=================");
                    printArea(area);
                #endif
                    break;
                }

                case Event::Type_E::EMERGENCY_LANE_OCCUPANCY:
                {
                    PMNMDetectRuleInfo_S stPMNMDetectRuleInfo;
                    stPMNMDetectRuleInfo.stRuleInfo = stRule;
                    stPMNMDetectRuleInfo.veDetectionTargets = rule.aDetectionTarget;
                    m_vstEmergencyLaneOccupancyRule.push_back(stPMNMDetectRuleInfo);
                #if HumanAreaDetect_DEBUG
                    dlog_debug("=====================打印应急车道区域area=================");
                    printArea(area);
                #endif
                    break;
                }
                
                case Event::Type_E::NON_MOTOR_VEHICLE_INTRUSION:
                {
                    PMNMDetectRuleInfo_S stPMNMDetectRuleInfo;
                    stPMNMDetectRuleInfo.stRuleInfo = stRule;
                    stPMNMDetectRuleInfo.veDetectionTargets = rule.aDetectionTarget;
                    m_vstNonMotorVehicleIntrusionRule.push_back(stPMNMDetectRuleInfo);
                #if HumanAreaDetect_DEBUG
                    dlog_debug("=====================打印非机动车闯入区域area=================");
                    printArea(area);
                #endif
                    break;
                }
                default:
                    break;
            }
        }
    }
}

#ifdef ENABLE_GAT1400_SRC
void CPMNMDetect::pushImageToGat1400(
    const cv::Mat &image, const std::vector<PMNMDetect_NS::Result_S> &vecResult)
{
    std::vector<uchar> buffer;
    security_subimage_info_t stImageInfo;
    int status = 0;
    if (image.empty()) {
        dlog_debug("image empty");
        return;
    }
    if (!cv::imencode(".jpg", image, buffer)) {
        dlog_debug("jpeg 编码失败");
        return;
    }
    if (!buffer.empty()) {
        stImageInfo.Data.resize(buffer.size());
        memcpy(&(stImageInfo.Data)[0], buffer.data(), buffer.size());
        stImageInfo.FileFormat = "Jpeg";
        stImageInfo.Width = image.cols;
        stImageInfo.Height = image.rows;
    }

    for (auto &result : vecResult) {
        if (!result.bEmergencyLaneOccupancyFlag
            && !result.bEntryFlag
            && !result.bIntrusionFlag
            && !result.bLeaveFlag
            && !result.bNonMotorVehicleIntrusionFlag
            && !result.enTripLineType
        ) continue;

        /* 避免侵入事件一直触发上传 */
        if (result.bIntrusionFlag && !m_IntruUploadInterval.isUpload()) {
            continue;
        }
        
        // 坐标点转换
        Common::PosF_S stPosition1 {result.fX1, result.fY1};
        Common::PosF_S stPosition2 {result.fX2, result.fY2};
        bool bConvert1 = stPosition1.ConvertResolution(m_nWidth, m_nHeight, image.cols, image.rows);
        bool bConvert2 = stPosition2.ConvertResolution(m_nWidth, m_nHeight, image.cols, image.rows);

        /* 判断上传类型 */
        /* 0-人 1-机动车 2-非机动车 */
        if (result.nID == 0) {
            security_persons_t stPersons;
            security_person_t stPerson;

            stPerson.InfoKind = SecurityInfoType::Auto;
            if (bConvert1 && bConvert2) {
                stPerson.LeftTopX = stPosition1.fX;
                stPerson.LeftTopY = stPosition1.fY;
                stPerson.RightBtmX = stPosition2.fX;
                stPerson.RightBtmY = stPosition2.fY;
            }
            stPerson.Behavior = BEHAVIOR_TYPE_OTHER;
            stImageInfo.Type = IMAGE_TYPE_SCENE;

            stPerson.SubImageList.push_back(stImageInfo);
            stPersons.push_back(stPerson);
            status = GAT1400::CGAT1400::instance()->uploadPersons(stPersons);
            if (status) {
                dlog_debug("uploadPersons faild %d", status);
            }
        } else if (result.nID == 1) {
            security_motorvehicles_t stVehicles;
            security_motorvehicle_t stVehicle;

            stVehicle.InfoKind = SecurityInfoType::Auto;
            if (bConvert1 && bConvert2) {
                stVehicle.LeftTopX = stPosition1.fX;
                stVehicle.LeftTopY = stPosition1.fY;
                stVehicle.RightBtmX = stPosition2.fX;
                stVehicle.RightBtmY = stPosition2.fY;
            }
            stImageInfo.Type = IMAGE_TYPE_MOTOR_VEHICLE;

            stVehicle.SubImageList.push_back(stImageInfo);
            stVehicles.push_back(stVehicle);
            status = GAT1400::CGAT1400::instance()->uploadMotorvehicles(stVehicles);
            if (status) {
                dlog_debug("uploadMotorvehicles faild %d", status);
            }
        } else if (result.nID == 2) {
            security_nonmotorvehicles_t stNonmotors;
            security_nonmotorvehicle_t stNonmotor;

            stNonmotor.InfoKind = SecurityInfoType::Auto;
            if (bConvert1 && bConvert2) {
                stNonmotor.LeftTopX = stPosition1.fX;
                stNonmotor.LeftTopY = stPosition1.fY;
                stNonmotor.RightBtmX = stPosition2.fX;
                stNonmotor.RightBtmY = stPosition2.fY;
            }
            stImageInfo.Type = IMAGE_TYPE_SCENE;

            stNonmotor.SubImageList.push_back(stImageInfo);
            stNonmotors.push_back(stNonmotor);
            status = GAT1400::CGAT1400::instance()->uploadNonmotorvehicles(stNonmotors);
            if (status) {
                dlog_debug("uploadNonmotorvehicles faild %d", status);
            }
        }
    }
}
#endif