/**
 * @file person_detect.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-05
 * 
 * @brief 机动车、行人、非机动车检测相关
 */

#include "person_detect.hpp"
#include "algo_stream_deal.h"
#include "StatisticsTimer.hpp"
#ifdef ENABLE_GAT1400_SRC
#include "gat1400.h"
#endif

/* 数据队列 */
#define QUEUE_MAX (2)
//人体检测目标
#define PERSON_DETECT_TARGET    0 

CPersonDetect::CPersonDetect()
 : m_dateQueue(QUEUE_MAX)
{
	m_bRunning.store(true);
    m_thread = std::thread(&CPersonDetect::run, this);
}

CPersonDetect::~CPersonDetect()
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
    unInitPlayPhoneDetect();
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


void CPersonDetect::recvMediaData(MediaData_S stMediaData)
{
	if (/*!m_stAlgoCrossCfg.bEnable && !m_stAlgoIntruCfg.bEnable && !m_stAlgoEntryCfg.bEnable && !m_stAlgoExitCfg.bEnable &&*/ 
        !m_stPhoneUsageCfg.bEnable && !m_stLoiteringCfg.bEnable && !m_stFenceClimbingCfg.bEnable && !m_stLeavePostCfg.bEnable && 
        !m_stPedestrianIntrusionCfg.bEnable && !m_stCrowdGatheringDetCfg.bEnable && !m_stPersonFallDownCfg.bEnable)
    {
        dlog_debug("ai_app: 行人检测-开关未启用");
        return;
    }

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("ai_app: 行人检测-数据队列满了");
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

void CPersonDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
	// m_stAlgoCrossCfg.bEnable = stAlgoConfig.nEnLineCrossing;
    // m_stAlgoIntruCfg.bEnable = stAlgoConfig.nEnIntrusion;
    // m_stAlgoEntryCfg.bEnable = stAlgoConfig.nEnEnterRegion;
    // m_stAlgoExitCfg.bEnable = stAlgoConfig.nEnLeaveRegion;
    m_stPhoneUsageCfg.bEnable = stAlgoConfig.nEnPhoneUsage;
    m_stLoiteringCfg.bEnable = stAlgoConfig.nEnLoiteringDetect;
    m_stFenceClimbingCfg.bEnable = stAlgoConfig.nEnFenceClimbing;
    m_stLeavePostCfg.bEnable = stAlgoConfig.nEnLeavePost;
    m_stPedestrianIntrusionCfg.bEnable = stAlgoConfig.nEnPedestrianIntrusion;
    m_stCrowdGatheringDetCfg.bEnable = stAlgoConfig.nEnCrowdGathering;
    m_stPersonFallDownCfg.bEnable = stAlgoConfig.nEnPersonFallDown;
#if 0
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
#endif
    if (m_stLoiteringCfg.bEnable)
    {
        Alarm::LoiteringDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
    if (m_stFenceClimbingCfg.bEnable)
    {
        Alarm::FenceClimbingDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
    if (m_stLeavePostCfg.bEnable)
    {
        Alarm::LeavePostDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
    if (m_stPedestrianIntrusionCfg.bEnable)
    {
        Alarm::PedestrianIntrusionDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
    if (m_stCrowdGatheringDetCfg.bEnable)
    {
        Alarm::CrowdGathering_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
    if (m_stPersonFallDownCfg.bEnable)
    {
        Alarm::PersonFallDownDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
    if(m_stPhoneUsageCfg.bEnable)
    {
		Alarm::PhoneUsageDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
}

#if 0
void CPersonDetect::setAlgoParamCfg(const Alarm::BoundaryDetection_S &stAlgoCfg,Event::Type_E enType)
{
	dlog_debug("ai_app:  设置越界检测参数");
    m_stAlgoCrossCfg = stAlgoCfg;
    convertBoundaryAndEnable(m_stAlgoCrossCfg);
}

void CPersonDetect::setAlgoParamCfg(const Alarm::FieldDetection_S &stAlgoCfg,Event::Type_E enType)
{
	dlog_debug("ai_app:  设置区域入侵参数");
    m_stAlgoIntruCfg = stAlgoCfg;
    m_vstIntruRule.clear();
    convertResolutionAndEnable(m_stAlgoIntruCfg,enType); 
}

void CPersonDetect::setAlgoParamCfg(const Alarm::EntranceDetection_S &stAlgoCfg,Event::Type_E enType)
{
	dlog_debug("ai_app:  设置进入区域参数");
    m_stAlgoEntryCfg = stAlgoCfg;
    m_vstEntryRule.clear();
    convertResolutionAndEnable(m_stAlgoEntryCfg,enType); 
}

void CPersonDetect::setAlgoParamCfg(const Alarm::ExitingDetection_S &stAlgoCfg,Event::Type_E enType)
{
	dlog_debug("ai_app:  设置离开区域参数");
    m_stAlgoExitCfg = stAlgoCfg;
    m_vstExitRule.clear();
    convertResolutionAndEnable(m_stAlgoExitCfg,enType); 
}
#endif

void CPersonDetect::setAlgoParamCfg(const Alarm::LoiteringDetection_S &stAlgoCfg,Event::Type_E enType)
{
    dlog_debug("ai_app:  设置徘徊侦测区域参数");
    m_stLoiteringCfg = stAlgoCfg;
    m_vstLoiteringRule.clear();
    convertResolutionAndEnable(m_stLoiteringCfg,enType); 
}

void CPersonDetect::setAlgoParamCfg(const Alarm::FenceClimbingDetection_S &stAlgoCfg,Event::Type_E enType)
{
    dlog_debug("ai_app:  设置翻越围栏参数");
    m_stFenceClimbingCfg = stAlgoCfg;
    m_vstFenceClimbingRule.clear();
    convertResolutionAndEnable(m_stFenceClimbingCfg,enType); 
}

void CPersonDetect::setAlgoParamCfg(const Alarm::LeavePostDetection_S &stAlgoCfg,Event::Type_E enType)
{
    dlog_debug("ai_app:  设置离岗识别参数");
    m_stLeavePostCfg = stAlgoCfg;
    m_vstLeavePostRule.clear();
    convertResolutionAndEnable(m_stLeavePostCfg,enType); 
}

void CPersonDetect::setAlgoParamCfg(const Alarm::PedestrianIntrusionDetection_S &stAlgoCfg,Event::Type_E enType)
{
    dlog_debug("ai_app:  设置行人闯入参数");
    m_stPedestrianIntrusionCfg = stAlgoCfg;
    m_vstPedestrianIntrusionRule.clear();
    convertResolutionAndEnable(m_stPedestrianIntrusionCfg,enType); 
}

void CPersonDetect::setAlgoParamCfg(const Alarm::CrowdGathering_S &stAlgoCfg,Event::Type_E enType)
{
    dlog_debug("ai_app:  设置人员聚集参数");
    m_stCrowdGatheringDetCfg = stAlgoCfg;
    m_vstCrowdGatheringDetRule.clear();
    convertCrowdGatherAndEnable(m_stCrowdGatheringDetCfg); 
}

void CPersonDetect::setAlgoParamCfg(const Alarm::PersonFallDownDetection_S &stAlgoCfg,Event::Type_E enType)
{
    dlog_debug("ai_app:  设置人员倒地参数");
    m_stPersonFallDownCfg = stAlgoCfg;
}

void CPersonDetect::setAlgoParamCfg(const Alarm::PhoneUsageDetection_S &stAlgoCfg,Event::Type_E enType)
{
    dlog_debug("ai_app: 设置玩手机识别参数");
    m_stPhoneUsageCfg = stAlgoCfg;
}

float CPersonDetect::sensitivityToConfidence(int sensitivity, float minConfidence, float maxConfidence) 
{
    int clampedSens = std::clamp(sensitivity, 1, 100);

    float confidence = maxConfidence - (clampedSens - 1) * (maxConfidence - minConfidence) / (100 - 1);

    confidence = std::clamp(confidence, minConfidence, maxConfidence);

    return confidence;
}

int CPersonDetect::sensitivityToFrames(int sensitivity, int minFrames, int maxFrames) 
{
    if (sensitivity <= 0) return maxFrames;
    if (sensitivity >= 100) return minFrames;
    
    // 线性映射
    double ratio = (100.0 - sensitivity) / 100.0;
    int frames = minFrames + static_cast<int>(ratio * (maxFrames - minFrames));
    
    return frames;
}

bool CPersonDetect::init()
{
    if (!m_pHumanAreaDetectHandle)
    {
        HumanAreaDetect_NS::InParam_S stInParam;
        stInParam.strModelPath = "/opt/cam/model/PersonDetectDetect.json";
        stInParam.bDebug = false;

        m_pHumanAreaDetectHandle = new HumanAreaDetect_NS::CHumanAreaDetectV3_0(stInParam);
        if (m_pHumanAreaDetectHandle)
        {
            if (m_pHumanAreaDetectHandle->init())
            {
                dlog_debug("ai_app: 行人检测算法初始化成功, %s", stInParam.strModelPath.c_str());
              
                return true;
            }
            else
            {
                delete m_pHumanAreaDetectHandle;
                m_pHumanAreaDetectHandle = nullptr;
                dlog_debug("边界检测算法初始化失败");
            }
        }
    }
    return false;
}

bool CPersonDetect::initPlayPhoneDetect()
{
    if (!m_pPlayPhoneDetectHandle)
    {
        PlayPhoneDetect_NS::InParam_S stInParam;
        stInParam.strModelPath = "/opt/cam/model/PlayPhoneDetect.json";
        stInParam.bDebug = false;

        m_pPlayPhoneDetectHandle = new PlayPhoneDetect_NS::CPlayPhoneDetectV1_0(stInParam);
        if (m_pPlayPhoneDetectHandle)
        {
            if (m_pPlayPhoneDetectHandle->init())
            {
                dlog_debug("ai_app:  玩手机识别算法初始化成功, %s", stInParam.strModelPath.c_str());
              
                return true;
            }
            else
            {
                delete m_pPlayPhoneDetectHandle;
                m_pPlayPhoneDetectHandle = nullptr;
                dlog_debug(" 玩手机识别算法初始化失败");
            }
        }
    }
    return false;
}
 
bool CPersonDetect::unInit()
{
    if (m_pHumanAreaDetectHandle)
    {
        delete m_pHumanAreaDetectHandle;
        m_pHumanAreaDetectHandle = nullptr;
    }
    
    return true;
}

bool CPersonDetect::unInitPlayPhoneDetect()
{
    if (m_pPlayPhoneDetectHandle)
    {
        delete m_pPlayPhoneDetectHandle;
        m_pPlayPhoneDetectHandle = nullptr;
    }
    
    return true;
}


void CPersonDetect::changeRuleInfos(const Event::RuleInfo &stRuleInfo, bool bNeedClear)
{
    if (bNeedClear)
    {
        m_vstRuleInfo.clear();
    }
    
    if (stRuleInfo.enType == Event::Type::LINE_CROSSING || stRuleInfo.enType == Event::Type::INTRUSION || 
        stRuleInfo.enType == Event::Type::ENTER_REGION || stRuleInfo.enType == Event::Type::LEAVE_REGION)
    {
        dlog_debug("ai_app:  更新边界检测划线数据 type[%d]", (int)stRuleInfo.enType);
        m_vstRuleInfo.push_back(stRuleInfo);
    }
}

void CPersonDetect::drawRulesToImage(cv::Mat& inMat)
{
    for (const auto& rule : m_vstRuleInfo)
    {
        /* 绘制所有的线条 */
        for (const auto& line : rule.lines)
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
        for (const auto& area : rule.areas)
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
int CPersonDetect::convertMaskToType( int eventFlags)
{
    // dlog_debug("ai_app: 边界检测事件 [%s]\n", std::bitset<4>(eventFlags).to_string().c_str());
    
    // { if (eventFlags & 0x01) return 8; /* Event::Type::LINE_CROSSING */ }
    // { if (eventFlags & 0x02) return 9; /* Event::Type::INTRUSION */ }
    // { if (eventFlags & 0x04) return 10; /* Event::Type::ENTER_REGION */ }
    // { if (eventFlags & 0x08) return 11; /* Event::Type::LEAVE_REGION */ }
    { if (eventFlags & 0x10) return 29; /* Event::Type::FENCE_CLIMBING */ }
    { if (eventFlags & 0x20) return 26; /* Event::Type::LEAVE_POST */ }
    { if (eventFlags & 0x40) return 39; /* Event::Type::PEDESTRIAN_INTRUSION */ }
    { if (eventFlags & 0x80) return 15; /* Event::Type::LOITERING_DETECT */ }
    { if (eventFlags & 0x100) return 16; /* Event::Type::CROWD_GATHERING */ }
    { if (eventFlags & 0x200) return 28; /* Event::Type::PERSON_FALL_DOWN */ }

    return -1;
}

void CPersonDetect::run()
{
    MediaData_S      stMediaData;
    std::vector<int> vResultData;
    vResultData.clear();   

    std::vector<HumanAreaDetect_NS::Result_S> vecResult;

    while (m_bRunning.load())
    {
        if (!m_pHumanAreaDetectHandle)
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

        if(m_stPhoneUsageCfg.bEnable)
        {
            if(!m_pPlayPhoneDetectHandle)
            {
                if (!initPlayPhoneDetect())
                {
                    dlog_error("等待玩手机检测初始化");
                    /* 延迟等待 1s */
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_condition.wait_for(lock, std::chrono::seconds(1), [this] {
                        return !m_bRunning.load();
                    });
                }
                continue;
            }
        }
        /* 阻塞获取 */
        m_dateQueue.pop(stMediaData, -1);
        if (stMediaData.nSize == 0)
        {
            /* 数据为空 */
            continue;
        }
      
         //dlog_debug("ai_app [行人检测]: 处理前Image分辨率[ %d × %d ] 格式[%d] 大小[%d]",
        //  stMediaData.stMediaParam.nVideoWidth,stMediaData.stMediaParam.nVideoHeight
        //  ,stMediaData.stMediaParam.enPixelFormat,stMediaData.nSize);

        CStatisticsTimer runTime("边界检测完整耗时");

        /* 送分析 */
        if (1)
        {
            frameRate("边界检测-分析数据", 5);

            HumanAreaDetect_NS::InData_S stInData {};
            HumanAreaDetect_NS::OutData_S stOutData;
            
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
            //cv::rotate(rgbMat, rgbMat, cv::ROTATE_180);

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
#if 0
                /* 越界规则 */
                if(m_stAlgoCrossCfg.bEnable)
                {
                    for (auto &CrossRule : m_vstCrossRule)
                    {
                        HumanAreaDetect_NS::TripLineParam_S  stTripLineParam;    
                        if (!CrossRule.lines.empty() && CrossRule.bEnable)
                        {
                            stTripLineParam.bEnable = true;
                            for (const auto& line : CrossRule.lines)
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
                        stInData.stParam.vstTripLineParam.push_back(stTripLineParam); 
                    }
                }

                /* 区域入侵规则 */
                if(m_stAlgoIntruCfg.bEnable)
                {
                    for (auto &IntruRule : m_vstIntruRule)
                    {
                        HumanAreaDetect_NS::IntrusionParam_S stIntrusionParam;    
                        if (!IntruRule.areas.empty() && IntruRule.bEnable)
                        {
                            stIntrusionParam.bEnable = true;
                            for (const auto& point : IntruRule.areas[0])
                            {
                                stIntrusionParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }
                        }
                        else
                        {
                            stIntrusionParam.bEnable = false;
                        }
                        stInData.stParam.vstIntrusionParam.push_back(stIntrusionParam); 
                    }
                }

                /* 进入区域规则 */
                if(m_stAlgoEntryCfg.bEnable)
                {
                    for (auto &EntryRule : m_vstEntryRule)
                    {
                        HumanAreaDetect_NS::EntryParam_S     stEntryParam;
                        if (!EntryRule.areas.empty() && EntryRule.bEnable)
                        {
                            stEntryParam.bEnable = true;
                            for (const auto& point : EntryRule.areas[0])
                            {
                                stEntryParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }
                        }
                        else
                        {
                            stEntryParam.bEnable = false;
                        }
                        stInData.stParam.vstEntryParam.push_back(stEntryParam); 
                    }
                }

                 /* 离开区域规则 */
                if(m_stAlgoExitCfg.bEnable)
                {
                    for (auto &ExitRule : m_vstExitRule)
                    {
                        HumanAreaDetect_NS::LeaveParam_S     stLeaveParam;
                        if (!ExitRule.areas.empty() && ExitRule.bEnable)
                        {
                            stLeaveParam.bEnable = true;
                            for (const auto& point : ExitRule.areas[0])
                            {
                                stLeaveParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }
                        }
                        else
                        {
                            stLeaveParam.bEnable = false;
                        }
                        stInData.stParam.vstLeaveParam.push_back(stLeaveParam); 
                    }
                }
#endif
                /* 徘徊侦测规则 */
                if(m_stLoiteringCfg.bEnable)
                {
                    for (auto &LoiteringRule : m_vstLoiteringRule)
                    {
                        HumanAreaDetect_NS::LoiteringParam_S     stLoiteringParam;
                        if (!LoiteringRule.areas.empty() && LoiteringRule.bEnable)
                        {
                            stLoiteringParam.bEnable = true;
                            stLoiteringParam.nTimeThreshold = LoiteringRule.nTimeThreshold;
                            for (const auto& point : LoiteringRule.areas[0])
                            {
                                stLoiteringParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }
                        }
                        else
                        {
                            stLoiteringParam.bEnable = false;
                        }
                        stInData.stParam.vsLoiteringParam.push_back(stLoiteringParam); 
                    }
                }
                /* 翻越围栏规则 */
                if(m_stFenceClimbingCfg.bEnable)
                {
                    for (auto &FenceClimbingRule : m_vstFenceClimbingRule)
                    {
                        HumanAreaDetect_NS::FenceClimbingParam_S     stFenceClimbingParam;
                        if (!FenceClimbingRule.areas.empty() && FenceClimbingRule.bEnable)
                        {
                            stFenceClimbingParam.bEnable = true;
                            for (const auto& point : FenceClimbingRule.areas[0])
                            {
                                stFenceClimbingParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }
                            /* 灵敏度转换检测帧数 */
                            stFenceClimbingParam.nDetectFrame = (100 - FenceClimbingRule.nSensitivity);
                            if(stFenceClimbingParam.nDetectFrame <= 0 || stFenceClimbingParam.nDetectFrame > 100)
                            {
                                stFenceClimbingParam.nDetectFrame = 1;
                            }
                        }
                        else
                        {
                            stFenceClimbingParam.bEnable = false;
                        }
                        stInData.stParam.vstFenceClimbingParam.push_back(stFenceClimbingParam); 
                    }
                }
                /* 离岗规则 */
                if(m_stLeavePostCfg.bEnable)
                {
                    for (auto &LeavePostRule : m_vstLeavePostRule)
                    {
                        HumanAreaDetect_NS::LeavePostParam_S     stLeavePostParam;
                        if (!LeavePostRule.areas.empty() && LeavePostRule.bEnable)
                        {
                            stLeavePostParam.bEnable = true;
                            stLeavePostParam.nTimeThreshold = LeavePostRule.nTimeThreshold;
                            for (const auto& point : LeavePostRule.areas[0])
                            {
                                stLeavePostParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }
                        }
                        else
                        {
                            stLeavePostParam.bEnable = false;
                        }
                        stInData.stParam.vstLeavePostParam.push_back(stLeavePostParam); 
                    }
                }
                /* 行人闯入识别规则 */
                if(m_stPedestrianIntrusionCfg.bEnable)
                {
                    for (auto &PedestrianIntrusionRule : m_vstPedestrianIntrusionRule)
                    {
                        HumanAreaDetect_NS::PedestrianIntrusionParam_S     stPedestrianIntrusionParam;
                        if (!PedestrianIntrusionRule.areas.empty() && PedestrianIntrusionRule.bEnable)
                        {
                            stPedestrianIntrusionParam.bEnable = true;
                            stPedestrianIntrusionParam.nTimeThreshold = PedestrianIntrusionRule.nTimeThreshold;
                            for (const auto& point : PedestrianIntrusionRule.areas[0])
                            {
                                stPedestrianIntrusionParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }
                        }
                        else
                        {
                            stPedestrianIntrusionParam.bEnable = false;
                        }
                        stInData.stParam.vstPedestrianIntrusionParam.push_back(stPedestrianIntrusionParam); 
                    }
                }

                /* 人员聚集识别规则 */
                if(m_stCrowdGatheringDetCfg.bEnable)
                {
                    for (auto &CrowdGatheringDetRule : m_vstCrowdGatheringDetRule)
                    {
                        HumanAreaDetect_NS::CrowdGatheringDetParam_S stCrowdGatheringDetParam;
                        if (!CrowdGatheringDetRule.areas.empty() && CrowdGatheringDetRule.bEnable)
                        {
                            stCrowdGatheringDetParam.bEnable = true;
                            stCrowdGatheringDetParam.nProportionThreshold = CrowdGatheringDetRule.nSensitivity;
                            for (const auto& point : CrowdGatheringDetRule.areas[0])
                            {
                                stCrowdGatheringDetParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }
                        }
                        else
                        {
                            stCrowdGatheringDetParam.bEnable = false;
                        }
                        stInData.stParam.vstCrowdGatheringDetParam.push_back(stCrowdGatheringDetParam); 
                    }

                }

                HumanAreaDetect_NS::PersonFallDownParam_S stPersonFallDownParam;
                /* 人员倒地识别 */
                if(m_stPersonFallDownCfg.bEnable)
                {
                    stPersonFallDownParam.bEnable = true;
                }
                stInData.stParam.stPersonFallDownParam = stPersonFallDownParam;

                /* 分析数据 */
                {
                    CStatisticsTimer runTime("边界检测算法耗时");
                    std::vector<HumanAreaDetect_NS::Result_S> vstModelDetectResult;
                    m_pHumanAreaDetectHandle->process(stInData, vecResult, vstModelDetectResult, &stOutData);
                    if(vstModelDetectResult.size() > 0 && m_stPhoneUsageCfg.bEnable)
                    {
                        detectPlayPhone(stInData.inMat, vstModelDetectResult);
                    }
 
                    if (stOutData.validResult)
                    {
                        for (const auto& result : vecResult)
                        {
                            int nType = convertMaskToType(stOutData.nType);
                            if (nType != -1 /*&& m_EventManager.handleEvent(0,  stOutData.nType)*/ )
                            {
                                /* 上报事件 */
                                dlog_debug("ai_app: 行人边界检测报警触发 类型[%d]",nType);
                                CEventLinkage::instance()->handleEvent(static_cast<Event::Type_E>(nType), false);
                            }
                        }
#ifdef ENABLE_GAT1400_SRC
                        // gat1400平台上传
                        Network::Gat1400Client_S stConfig;
                        GAT1400::CGAT1400::instance()->getGat1400Config(stConfig);
                        if (stConfig.enableGat1400 && stConfig.enablePerson && !vecResult.empty()) {
                            cv::Mat imageMat;
                            cv::cvtColor(i420Mat, imageMat, cv::COLOR_YUV2BGR_NV12);
                            pushPersonImageToGat1400(imageMat, vecResult);
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

void CPersonDetect::detectPlayPhone(cv::Mat &srcDate, std::vector<HumanAreaDetect_NS::Result_S> &vstModelDetectResult)
{
    if(srcDate.empty())
    {
        return ;
    }
    for(unsigned int i = 0; i < vstModelDetectResult.size(); i++)
    {
        PlayPhoneDetect_NS::InData_S stInData {};
        PlayPhoneDetect_NS::OutData_S stOutData;
        std::vector<PlayPhoneDetect_NS::Result_S> vecResult;

        stOutData.nType = 0;
        stOutData.validResult = false;
        cv::Rect roi(static_cast<int>(vstModelDetectResult[i].fX), static_cast<int>(vstModelDetectResult[i].fY), static_cast<int>(vstModelDetectResult[i].fWidth), static_cast<int>(vstModelDetectResult[i].fHeight));
        cv::Mat cropped = srcDate(roi).clone();

        cv::resize(
            cropped,          
            stInData.inMat,              
            cv::Size(m_nWidth, m_nHeight),  
            0, 0,                    
            cv::INTER_LINEAR 
        );
        if (!stInData.inMat.empty())
        {

            if(m_stPhoneUsageCfg.bEnable)
            {
                stInData.stParam.stPlayPhoneParam.bEnable = true;
                stInData.stParam.stPlayPhoneParam.fConfidence = sensitivityToConfidence(m_stPhoneUsageCfg.stRule.nSensitivity);
                stInData.stParam.stPlayPhoneParam.nDetectFrame = sensitivityToFrames(m_stPhoneUsageCfg.stRule.nSensitivity);
            }

            if (access("/playPhone_debugImage", F_OK) == 0)
            {
                using namespace std::chrono;
                uint64_t llTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                std::string strFileName(std::to_string(llTime));
                std::string strFilePath = std::string("/opt/algo/Image/") + "test_algo_" + strFileName + ".jpg";
                dlog_debug("============>debugImage [%s]", strFilePath.c_str());
                cv::imwrite(strFilePath, stInData.inMat);
                dlog_debug(" (%f, %f)(%f, %f) %d 灵敏度:%d %f 结果:%d", vstModelDetectResult[i].fX, vstModelDetectResult[i].fY, vstModelDetectResult[i].fWidth, vstModelDetectResult[i].fHeight, vstModelDetectResult.size(), m_stPhoneUsageCfg.stRule.nSensitivity, stInData.stParam.stPlayPhoneParam.fConfidence, vecResult.size());
            }

            CStatisticsTimer runTime(" 玩手机识别算法耗时");

            m_pPlayPhoneDetectHandle->process(stInData, vecResult, &stOutData);

            if (stOutData.validResult)
            {
                /* 上报事件 */
                dlog_debug("ai_app:  玩手机识别报警触发");
                CEventLinkage::instance()->handleEvent(Event::Type::PHONE_USAGE, false);
                break;
            }

        }

    }

    return ;
}

void CPersonDetect::convertBoundaryAndEnable(Alarm::BoundaryDetection_S &stConfig)
{
    if (!stConfig.aRule.empty())
    {
        bool bIsInit = false;
        m_vstCrossRule.clear();
        /* 转换警戒线坐标分辨率至算法分辨率 */
        for (auto &rule : stConfig.aRule)
        {
            Event::RuleInfo stRule;
            /* 转换起始点坐标 */
            float scaleX = static_cast<float>(m_nWidth) / PIXEL_WIDTH_1920;
            float scaleY = static_cast<float>(m_nHeight) / PIXEL_HEIGHT_1080;
            
            rule.stStartPos.fX *= scaleX;
            rule.stStartPos.fY *= scaleY;
            rule.stEndPos.fX *= scaleX;
            rule.stEndPos.fY *= scaleY;
            
            /* 判断是否设置了有效的警戒线 */
            if ((rule.stStartPos.fX != rule.stEndPos.fX) || 
                (rule.stStartPos.fY != rule.stEndPos.fY) && 
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
                /* 判断是否开启了人体检测 */
                bool bHasPerson = std::find(rule.aDetectionTarget.begin(), rule.aDetectionTarget.end(), PERSON_DETECT_TARGET) != rule.aDetectionTarget.end();
                if(!bHasPerson)
                {
                    stRule.bEnable = false;
                    dlog_debug("ai_app:  当前越界检测规则未开启人体检测");
                    continue;
                }
                else
                {
                    stRule.bEnable = true;
                    stRule.lines.push_back(line);
                }
            }
            if(stRule.bEnable)
            {
                m_vstCrossRule.push_back(stRule);
            }
        }
    }
}

void CPersonDetect::convertCrowdGatherAndEnable(Alarm::CrowdGathering_S &stAlgoCfg)
{
    dlog_debug("ai_app: 设置人员聚集侦测参数");
    
    /* 是否初始化 */
    bool bIsInit = false;

    if (!stAlgoCfg.aRule.empty())
    {
        /* 转换区域坐标分辨率至算法分辨率 */
        for (auto &rule : stAlgoCfg.aRule)
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
            else 
            {
                stRule.bEnable = true;
            }

            rule.stRegion.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight);
            
            stRule.nSensitivity = rule.nObjectOccup;
            stRule.nTimeThreshold = 0;
            dlog_debug("ai_app:  当前规则获取到 灵敏度[%d]", stRule.nSensitivity);
            for (auto &pos : rule.stRegion.aPoint)
            {
                ::Event::Point_S stPoint;
                stPoint.nX = pos.fX;
                stPoint.nY = pos.fY;
                area.push_back(stPoint);
            }
            stRule.areas.push_back(area);

            m_vstCrowdGatheringDetRule.push_back(stRule);
            bIsInit = true;
#if HumanAreaDetect_DEBUG
            dlog_debug("=====================打印进入区域area=================");
            printArea(area);
#endif
        }
    }
    stAlgoCfg.bEnable = bIsInit;
}

template<typename T>
void CPersonDetect::convertResolutionAndEnable(T &stConfig,Event::Type_E enType)
{
    if (!stConfig.aRule.empty())
    {
        /* 是否有任何一个区域初始化成功 */
        bool bIsInit = false;
        
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
            bool bHasPerson = true;
            if(enType == Event::Type_E::INTRUSION || enType == Event::Type_E::ENTER_REGION || enType == Event::Type_E::LEAVE_REGION)
            {
                bHasPerson = std::find(rule.aDetectionTarget.begin(), rule.aDetectionTarget.end(), PERSON_DETECT_TARGET) != rule.aDetectionTarget.end();
            }

            if(!bHasPerson)
            {
                stRule.bEnable = false;
                dlog_debug("ai_app:  当前规则未开启人体检测");
            }
            else
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
                    m_vstIntruRule.push_back(stRule);
                #if HumanAreaDetect_DEBUG
                    dlog_debug("=====================打印区域入侵area=================");
                    printArea(area);
                #endif
                    break;
                }
                
                case Event::Type_E::ENTER_REGION:
                {
                    m_vstEntryRule.push_back(stRule);
                 #if HumanAreaDetect_DEBUG
                    dlog_debug("=====================打印进入区域area=================");
                    printArea(area);
                 #endif
                    break;
                }
                
                case Event::Type_E::LEAVE_REGION:
                {
                    m_vstExitRule.push_back(stRule);
                #if HumanAreaDetect_DEBUG
                    dlog_debug("=====================打印离开区域area=================");
                    printArea(area);
                 #endif
                    break;
                }
                case Event::Type_E::LOITERING_DETECT:
                {
                    m_vstLoiteringRule.push_back(stRule);
                #if HumanAreaDetect_DEBUG
                    dlog_debug("=====================打印徘徊侦测区域area=================");
                    printArea(area);
                 #endif
                    break;
                }
                case Event::Type_E::FENCE_CLIMBING:
                {
                    m_vstFenceClimbingRule.push_back(stRule);
                #if HumanAreaDetect_DEBUG
                    dlog_debug("=====================打印翻越围栏区域area=================");
                    printArea(area);
                 #endif
                    break;
                }
                case Event::Type_E::LEAVE_POST:
                {
                    m_vstLeavePostRule.push_back(stRule);
                #if HumanAreaDetect_DEBUG
                    dlog_debug("=====================打印离岗识别区域area=================");
                    printArea(area);
                 #endif
                    break;
                }
                case Event::Type_E::PEDESTRIAN_INTRUSION:
                {
                    m_vstPedestrianIntrusionRule.push_back(stRule);
                #if HumanAreaDetect_DEBUG
                    dlog_debug("=====================打印行人闯入区域area=================");
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
void CPersonDetect::pushPersonImageToGat1400(const cv::Mat &image, const std::vector<HumanAreaDetect_NS::Result_S> &vecResult)
{
    security_persons_t stPersons;
    std::vector<uchar> buffer;
    if (image.empty()) {
        dlog_debug("image empty");
        return;
    }
    if (!cv::imencode(".jpg", image, buffer)) {
        dlog_debug("jpeg 编码失败");
        return;
    }
    
    for (auto &result : vecResult) {
        security_person_t stPerson;
        security_subimage_info_t stImageInfo;

        stPerson.InfoKind = SecurityInfoType::Auto;
        // 坐标点转换
        Common::PosF_S stPosition1 {result.fX, result.fY};
        Common::PosF_S stPosition2 {result.fX + result.fWidth, result.fY + result.fHeight};
        bool bConvert1 = stPosition1.ConvertResolution(m_nWidth, m_nHeight, image.cols, image.rows);
        bool bConvert2 = stPosition2.ConvertResolution(m_nWidth, m_nHeight, image.cols, image.rows);
        if (bConvert1 && bConvert2) {
            stPerson.LeftTopX = stPosition1.fX;
            stPerson.LeftTopY = stPosition1.fY;
            stPerson.RightBtmX = stPosition2.fX;
            stPerson.RightBtmY = stPosition2.fY;
        }
        
        if (result.bLoiteringFlag) {
            stPerson.Behavior = BEHAVIOR_TYPE_LOITERING;
        } else if (result.bCrowdGatheringDetParamFlag) {
            stPerson.Behavior = BEHAVIOR_TYPE_OTHER;
            stPerson.BehaviorDescription = "人员聚集";
        } else {
            stPerson.Behavior = BEHAVIOR_TYPE_OTHER;
        }

        if (!buffer.empty()) {
            stImageInfo.Data.resize(buffer.size());
            memcpy(&(stImageInfo.Data)[0], buffer.data(), buffer.size());
            stImageInfo.Type = IMAGE_TYPE_SCENE;
            stImageInfo.FileFormat = "Jpeg";
            stImageInfo.Width = image.cols;
            stImageInfo.Height = image.rows;
        }

        stPerson.SubImageList.push_back(stImageInfo);
        stPersons.push_back(stPerson);
    }

    int status = GAT1400::CGAT1400::instance()->uploadPersons(stPersons);
    if (status) {
        dlog_debug("upload person faild %d", status);
    }
}
#endif