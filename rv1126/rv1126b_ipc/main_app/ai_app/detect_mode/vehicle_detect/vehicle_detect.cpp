/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-11-27 17:24:46
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-12-10 09:41:45
 * @FilePath: /1126/rv1126b_ipc/main_app/ai_app/detect_mode/vehicle_detect/vehicle_detect.cpp
 * @Description: 车辆检测
 */

#include "vehicle_detect.hpp"
#include "algo_stream_deal.h"
#include "StatisticsTimer.hpp"
#ifdef ENABLE_GAT1400_SRC
#include "gat1400.h"
#endif

/* 数据队列 */
#define QUEUE_MAX (2)

CVehicleDetect::CVehicleDetect()
 : m_dateQueue(QUEUE_MAX)
{
	m_bRunning.store(true);
    m_thread = std::thread(&CVehicleDetect::run, this);
}

CVehicleDetect::~CVehicleDetect()
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


void CVehicleDetect::recvMediaData(MediaData_S stMediaData)
{
	if (!m_stDrivingAgainstTrafficDetectionCfg.bEnable && !m_stAlgoIllegalLaneChangeDetectionCfg.bEnable && !m_stAlgoCongestionDetectionCfg.bEnable && !m_stAlgoParkingDetectionCfg.bEnable)
    {
        dlog_debug("ai_app: 车辆检测-开关未启用");
        return;
    }

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("ai_app: 车辆检测-数据队列满了");
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

void CVehicleDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stDrivingAgainstTrafficDetectionCfg.bEnable = stAlgoConfig.nEnReverseDirection;
    m_stAlgoCongestionDetectionCfg.bEnable = stAlgoConfig.nEnCongestion;
    m_stAlgoParkingDetectionCfg.bEnable = stAlgoConfig.nEnParkingDetect;
    // stAlgoConfig.nEnIllegalParking
    m_stAlgoIllegalLaneChangeDetectionCfg.bEnable = stAlgoConfig.nEnIllegalLaneChange;  

    if (m_stDrivingAgainstTrafficDetectionCfg.bEnable)
    {
		Alarm::DrivingAgainstTrafficDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo, Event::Type::REVERSE_DIRECTION);
    }

    if(m_stAlgoCongestionDetectionCfg.bEnable)
    {
        Alarm::CongestionDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo, Event::Type::CONGESTION);
    }

    if(m_stAlgoParkingDetectionCfg.bEnable)
    {
        Alarm::ParkingDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo, Event::Type::PARKING_DETECT);
    }

    if (m_stAlgoIllegalLaneChangeDetectionCfg.bEnable)
    {
        Alarm::IllegalLaneChangeDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo, Event::Type::ILLEGAL_LANE_CHANGE);
    }

    return ;
}

void CVehicleDetect::setAlgoParamCfg(const Alarm::DrivingAgainstTrafficDetection_S &stAlgoCfg, Event::Type_E enType)
{
	dlog_debug("ai_app:  设置逆行检测参数");
    m_stDrivingAgainstTrafficDetectionCfg = stAlgoCfg;
    convertAlertLineToZoneAndIsEnable(m_stDrivingAgainstTrafficDetectionCfg, enType);
    return ;
}

void CVehicleDetect::setAlgoParamCfg(const Alarm::CongestionDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置拥堵识别检测参数");
    m_stAlgoCongestionDetectionCfg = stAlgoCfg;
    return;
}

void CVehicleDetect::setAlgoParamCfg(const Alarm::ParkingDetection_S &stAlgoCfg, Event::Type_E enType)
{
	dlog_debug("ai_app:  设置违停参数");
    m_stAlgoParkingDetectionCfg = stAlgoCfg;
    m_vstIllegalParkingRule.clear();
    convertParkingAndEnable(m_stAlgoParkingDetectionCfg, enType); 
    return ;
}

void CVehicleDetect::setAlgoParamCfg(const Alarm::IllegalLaneChangeDetection_S &stAlgoCfg, Event::Type_E enType)
{
	dlog_debug("ai_app:  设置违规变道参数");
    m_stAlgoIllegalLaneChangeDetectionCfg = stAlgoCfg;
    m_vstIllegalLaneChangeRule.clear();
    convertAlertLineToZoneAndIsEnable(m_stAlgoIllegalLaneChangeDetectionCfg, enType); 
    return ;
}

bool CVehicleDetect::init()
{
    if (!m_pVehicleDetectHandle)
    {
        VehicleDetect_NS::InParam_S stInParam;
        stInParam.strModelPath = "/opt/cam/model/VehicleDetect.json";
        stInParam.bDebug = false;
        stInParam.strAnalyzeDataPath = "/mnt/VehicleDetect";

        m_pVehicleDetectHandle = new VehicleDetect_NS::CVehicleDetectV2_0(stInParam);
        if (m_pVehicleDetectHandle)
        {
            if (m_pVehicleDetectHandle->init())
            {
                dlog_debug("ai_app: 车辆检测算法初始化成功, %s", stInParam.strModelPath.c_str());
              
                return true;
            }
            else
            {
                delete m_pVehicleDetectHandle;
                m_pVehicleDetectHandle = nullptr;
                dlog_debug("车辆检测算法初始化失败");
            }
        }
    }
    return false;
}
 
bool CVehicleDetect::unInit()
{
    if (m_pVehicleDetectHandle)
    {
        delete m_pVehicleDetectHandle;
        m_pVehicleDetectHandle = nullptr;
    }
    
    return true;
}


// void CVehicleDetect::changeRuleInfos(const Event::RuleInfo &stRuleInfo, bool bNeedClear)
// {
//     if (bNeedClear)
//     {
//         m_vstRuleInfo.clear();
//     }
    
//     if (stRuleInfo.enType == Event::Type::PARKING_DETECT || stRuleInfo.enType == Event::Type::ILLEGAL_LANE_CHANGE || 
//         stRuleInfo.enType == Event::Type::REVERSE_DIRECTION)
//     {
//         dlog_debug("ai_app:  更新车辆检测划线数据 type[%d]", (int)stRuleInfo.enType);
//         m_vstRuleInfo.push_back(stRuleInfo);
//     }
// }

void CVehicleDetect::drawRulesToImage(cv::Mat& inMat)
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

float CVehicleDetect::sensitivityToConfidence(int sensitivity, float minConfidence, float maxConfidence) 
{
    int clampedSens = std::clamp(sensitivity, 1, 100);

    float confidence = maxConfidence - (clampedSens - 1) * (maxConfidence - minConfidence) / (100 - 1);

    confidence = std::clamp(confidence, minConfidence, maxConfidence);

    return confidence;
}

/* 事件转换函数 */
int CVehicleDetect::convertMaskToType( int eventFlags, VehicleDetectEventType_S &stEventStatus)
{
    // dlog_debug("ai_app: 车辆检测事件 [%s]\n", std::bitset<4>(eventFlags).to_string().c_str());
    
    /* Event::Type::REVERSE_DIRECTION */
    if (eventFlags & 0x01)
    {
        stEventStatus.bDrivingAgainstTraffic = true;
    }

    /* Event::Type::CONGESTION */
    if (eventFlags & 0x02) 
    {
        stEventStatus.bCongestion = true;
    }

    /* Event::Type::PARKING_DETECT */
    if (eventFlags & 0x04) 
    {
        stEventStatus.bIllegalParking = true;
    }

    /* Event::Type::ILLEGAL_LANE_CHANGE */
    if (eventFlags & 0x08) 
    {
        stEventStatus.bIllegalLaneChange = true;
    }

    return 0;
}

void CVehicleDetect::run()
{
    MediaData_S      stMediaData;

    while (m_bRunning.load())
    {
        if (!m_pVehicleDetectHandle)
        {
            if (!init())
            {
                dlog_error("等待车辆检测初始化");
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
         //dlog_debug("ai_app [车辆检测]: 处理前Image分辨率[ %d × %d ] 格式[%d] 大小[%d]",
        //  stMediaData.stMediaParam.nVideoWidth,stMediaData.stMediaParam.nVideoHeight
        //  ,stMediaData.stMediaParam.enPixelFormat,stMediaData.nSize);

        CStatisticsTimer runTime("车辆检测完整耗时");
        /* 送分析 */
        if (1)
        {
            frameRate("车辆检测-分析数据", 5);

            VehicleDetect_NS::InData_S stInData {};
            VehicleDetect_NS::OutData_S stOutData;
            
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

            /* 分辨率大小转换 */
            cv::resize(
                rgbMat,          
                stInData.inMat,              
                cv::Size(m_nWidth, m_nHeight),  
                0, 0,                    
                cv::INTER_LINEAR 
            );

            // cv::rotate(stInData.inMat, stInData.inMat, cv::ROTATE_180);

            if (!stInData.inMat.empty())
            {
                if (access("/vehicle_debugImage", F_OK) == 0)
                {
                    dlog_debug("============>debugImage");
                    cv::imwrite("/opt/algo/Image/vehicle_debugImage.jpg", stInData.inMat);
                }
                /* 逆行规则 */
                if(m_stDrivingAgainstTrafficDetectionCfg.bEnable)
                {
                    for (auto &DrivingAgainstTrafficRule : m_vstDrivingAgainstTrafficRule)
                    {
                        VehicleDetect_NS::DrivingAgainstTrafficParam_S  stDrivingAgainstTrafficParam;    
                        if (!DrivingAgainstTrafficRule.stRuleInfo.lines.empty() && DrivingAgainstTrafficRule.stRuleInfo.bEnable)
                        {
                            stDrivingAgainstTrafficParam.bEnable = true;
                            for (const auto& line : DrivingAgainstTrafficRule.stRuleInfo.lines)
                            {
                                if (line.size() >= 2)
                                {       
                                    stDrivingAgainstTrafficParam.alertLine1 = cv::Point(line[0].nX, line[0].nY);
                                    stDrivingAgainstTrafficParam.alertLine2 = cv::Point(line[1].nX, line[1].nY);
                                    stDrivingAgainstTrafficParam.eTripLineType = (VehicleDetect_NS::TripLineType_E)DrivingAgainstTrafficRule.enCrossDirection;
                                    stDrivingAgainstTrafficParam.fDrivingAgainstTrafficBoxThreshold = sensitivityToConfidence(DrivingAgainstTrafficRule.stRuleInfo.nSensitivity);
                                    if (access("/PrintDrivingAgainstTrafficDetectionCfg", F_OK) == 0)
                                    {
                                        dlog_debug("fCongestionBoxThreshold = %f", stDrivingAgainstTrafficParam.fDrivingAgainstTrafficBoxThreshold);
                                    }
                                }
                            }
                        }
                        else
                        {
                            stDrivingAgainstTrafficParam.bEnable = false;
                        }
                        stInData.stParam.vstDrivingAgainstTrafficParam.push_back(stDrivingAgainstTrafficParam); 
                    }
                }

                /* 拥堵识别 */
                if(m_stAlgoCongestionDetectionCfg.bEnable)
                {                    
                    stInData.stParam.stCongestionParam.fCongestionBoxThreshold = sensitivityToConfidence(m_stAlgoCongestionDetectionCfg.stRule.nSensitivity);
                    stInData.stParam.stCongestionParam.nCongestionThreshold = 100.0f * stInData.stParam.stCongestionParam.fCongestionBoxThreshold;
                    if (access("/PrintCongestionDetectionCfg", F_OK) == 0)
                    {
                        dlog_debug("fCongestionBoxThreshold = %f  nCongestionThreshold = %d ", stInData.stParam.stCongestionParam.fCongestionBoxThreshold, stInData.stParam.stCongestionParam.nCongestionThreshold);
                    }
                }

                /* 违规停车规则 */
                if(m_stAlgoParkingDetectionCfg.bEnable)
                {
                    for (auto &IllegalParkingRule : m_vstIllegalParkingRule)
                    {
                        VehicleDetect_NS::ParkingParam_S stParkingParam;
                        if (!IllegalParkingRule.areas.empty() && IllegalParkingRule.bEnable)
                        {
                            stParkingParam.bEnable = true;
                            stParkingParam.nParkingTimeThreshold = IllegalParkingRule.nTimeThreshold * 1000; /* s转换为ms */
                            stParkingParam.fParkingBoxThreshold = sensitivityToConfidence(IllegalParkingRule.nSensitivity);
                            for (const auto& point : IllegalParkingRule.areas[0])
                            {
                                stParkingParam.vecPoints.push_back(cv::Point(point.nX, point.nY));
                            }

                            if (access("/PrintParkingDetectionCfg", F_OK) == 0)
                            {
                                dlog_debug(" nParkingTimeThreshold = %d,  fParkingBoxThreshold = %f", stParkingParam.nParkingTimeThreshold, stParkingParam.fParkingBoxThreshold);
                            }
                        }
                        else
                        {
                            stParkingParam.bEnable = false;
                        }
                        stInData.stParam.vstParkingParam.push_back(stParkingParam); 
                    }
                }

                /* 违规变道规则 */
                if(m_stAlgoIllegalLaneChangeDetectionCfg.bEnable)
                {
                    for (auto &IllegalLaneChangeRule : m_vstIllegalLaneChangeRule)
                    {
                        VehicleDetect_NS::IllegalLaneChangeParam_S  stIllegalLaneChangeParam;    
                        if (!IllegalLaneChangeRule.stRuleInfo.lines.empty() && IllegalLaneChangeRule.stRuleInfo.bEnable)
                        {
                            stIllegalLaneChangeParam.bEnable = true;
                            for (const auto& line : IllegalLaneChangeRule.stRuleInfo.lines)
                            {
                                if (line.size() >= 2)
                                {       
                                    stIllegalLaneChangeParam.alertLine1 = cv::Point(line[0].nX, line[0].nY);
                                    stIllegalLaneChangeParam.alertLine2 = cv::Point(line[1].nX, line[1].nY);
                                    stIllegalLaneChangeParam.fIllegalLaneChangeBoxThreshold = sensitivityToConfidence(IllegalLaneChangeRule.stRuleInfo.nSensitivity);
                                    if (access("/PrintIllegalLaneChangeDetectionCfg", F_OK) == 0)
                                    {
                                        dlog_debug("fIllegalLaneChangeBoxThreshold = %f", stIllegalLaneChangeParam.fIllegalLaneChangeBoxThreshold);
                                    }
                                }
                            }
                        }
                        else
                        {
                            stIllegalLaneChangeParam.bEnable = false;
                        }
                        stInData.stParam.vstIllegalLaneChangeParam.push_back(stIllegalLaneChangeParam); 
                    }
                }
                
                /* 分析数据 */
                {
                    std::vector<VehicleDetect_NS::Result_S> vecResult;
                    
                    CStatisticsTimer runTime("车辆检测算法耗时");
                    m_pVehicleDetectHandle->process(stInData, vecResult, &stOutData);
#if 0
                    std::vector<VehicleDetect_NS::Result_S> vecResult1;
                    std::vector<Common::RectInfo_S> vstRectInfo;
                    m_pVehicleDetectHandle->process(stInData, vecResult, vecResult1, &stOutData);
                    for(unsigned int i = 0; i < vecResult1.size(); i++)
                    {
                        Common::RectInfo_S stRectInfo;
                        stRectInfo.nX1 = (int)vecResult1.at(i).fX;
                        stRectInfo.nY1 = (int)vecResult1.at(i).fY;
                        stRectInfo.nX2 = (int)(vecResult1.at(i).fX + vecResult1.at(i).fWidth);
                        stRectInfo.nY2 = (int)(vecResult1.at(i).fY + vecResult1.at(i).fHeight);
                        vstRectInfo.push_back(stRectInfo);
                    }

                    if(vstRectInfo.size() > 0)
                    {
                        send_detectionResult_to_osd(m_nWidth, m_nHeight, vstRectInfo);
                    }
#endif
#ifdef ENABLE_GAT1400_SRC
                    processVehicleDetect(stOutData.nType);
                    // 上传gat1400预处理
                    preProcessGat1400(i420Mat, vecResult, stOutData.nType);
#endif
                }
            }
            else
            {
                dlog_error("ai_app: 图片数据为空");
            }
        }
        else
        {
            dlog_error("ai_app: 车辆检测-获取虚拟地址失败");
        }
    }
}

template<typename T>
void CVehicleDetect::convertAlertLineToZoneAndIsEnable(T &stConfig, Event::Type_E enType)
{
    if (!stConfig.aRule.empty())
    {
        m_vstDrivingAgainstTrafficRule.clear();
        /* 转换警戒线坐标分辨率至算法分辨率坐标 */
        for (auto &rule : stConfig.aRule)
        {
            bool bIsInit = false;
            TrafficRuleInfo_S stRule;
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
                dlog_debug("ai_app: 车辆检测-逆行检测警戒线无效");
                stRule.stRuleInfo.bEnable = false;
            }
            else
            {
                dlog_debug("ai_app: 车辆检测-逆行检测警戒线有效");
                std::vector<::Event::Point_S> line;
                ::Event::Point_S stPoint;
                stPoint.nX = rule.stStartPos.fX;
                stPoint.nY = rule.stStartPos.fY;
                line.push_back(stPoint);
                stPoint.nX = rule.stEndPos.fX;
                stPoint.nY = rule.stEndPos.fY;
                line.push_back(stPoint);
                stRule.stRuleInfo.bEnable = true;
                stRule.stRuleInfo.nSensitivity = rule.nSensitivity;
                stRule.stRuleInfo.lines.push_back(line);
                
            }
            if(stRule.stRuleInfo.bEnable)
            {
                if(enType == Event::Type_E::REVERSE_DIRECTION)
                {
                    stRule.enCrossDirection = rule.enCrossDirection;
                    m_vstDrivingAgainstTrafficRule.push_back(stRule);
                }
                else 
                {
                    stRule.enCrossDirection = Alarm::CrossDirection_E::CROSS_DIRECTION_INVALID;
                    m_vstIllegalLaneChangeRule.push_back(stRule);
                }
                
            }
        }
    }
}

void CVehicleDetect::convertParkingAndEnable(Alarm::ParkingDetection_S &stAlgoCfg, Event::Type_E enType)
{
    dlog_debug("ai_app: 设置停车侦测参数");
    
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
            
            stRule.nSensitivity = rule.nSensitivity;
            stRule.nTimeThreshold = rule.nTimeThreshold;

            dlog_debug("ai_app:  当前规则获取到 灵敏度[%d]", stRule.nSensitivity);
            for (auto &pos : rule.stRegion.aPoint)
            {
                ::Event::Point_S stPoint;
                stPoint.nX = pos.fX;
                stPoint.nY = pos.fY;
                area.push_back(stPoint);
            }
            stRule.areas.push_back(area);
            stRule.enType = enType;

            m_vstIllegalParkingRule.push_back(stRule);
            bIsInit = true;
#if VehicleDetect_DEBUG
            dlog_debug("=====================打印违停区域area=================");
            printArea(area);
#endif
        }
    }
    stAlgoCfg.bEnable = bIsInit;
}

void CVehicleDetect::processVehicleDetect(int eventFlags) 
{
    VehicleDetectEventType_S stEventStatus;

    convertMaskToType(eventFlags, stEventStatus);

    if(m_stDrivingAgainstTrafficDetectionCfg.bEnable)
    {
        m_DrivingAgainstTrafficStateMachine.handleAlarmState(stEventStatus.bDrivingAgainstTraffic, Event::Type_E::REVERSE_DIRECTION);
    }
    if(m_stAlgoCongestionDetectionCfg.bEnable)
    {
        m_CongestionStateMachine.handleAlarmState(stEventStatus.bCongestion, Event::Type_E::CONGESTION);
    }
    if(m_stAlgoParkingDetectionCfg.bEnable)
    {
        m_IllegalParkingStateMachine.handleAlarmState(stEventStatus.bIllegalParking, Event::Type_E::PARKING_DETECT);
    }
    if(m_stAlgoIllegalLaneChangeDetectionCfg.bEnable)
    {
        m_IllegalLaneChangeStateMachine.handleAlarmState(stEventStatus.bIllegalLaneChange, Event::Type_E::ILLEGAL_LANE_CHANGE);
    }

    return ;
}

#ifdef ENABLE_GAT1400_SRC
void CVehicleDetect::pushVehiclesToGat1400(const cv::Mat &image, const std::vector<VehicleDetect_NS::Result_S> &vecResult)
{
    security_motorvehicles_t stVehicles;
    security_subimage_info_t stImageInfo;
    std::vector<uchar> buffer;
    if (image.empty()) {
        dlog_debug("iamge empty");
        return;
    }
    if (!cv::imencode(".jpg", image, buffer)) {
        dlog_debug("jpeg 编码失败");
        return;
    }

    if (!buffer.empty()) {
        stImageInfo.Data.resize(buffer.size());
        memcpy(&(stImageInfo.Data)[0], buffer.data(), buffer.size());
        stImageInfo.Type = IMAGE_TYPE_MOTOR_VEHICLE;
        stImageInfo.FileFormat = "Jpeg";
        stImageInfo.Width = image.cols;
        stImageInfo.Height = image.rows;
    }

    for (auto &result : vecResult) {
        // 停车触发上传
        if (result.bParkingFlag) {
            security_motorvehicle_t stVehicle;
            stVehicle.InfoKind = SecurityInfoType::Auto;
            // 坐标点转换
            Common::PosF_S stPosition1 {result.fX, result.fY};
            Common::PosF_S stPosition2 {result.fX + result.fWidth, result.fY + result.fHeight};
            bool bConvert1 = stPosition1.ConvertResolution(m_nWidth, m_nHeight, image.cols, image.rows);
            bool bConvert2 = stPosition2.ConvertResolution(m_nWidth, m_nHeight, image.cols, image.rows);

            if (bConvert1 && bConvert2) {
                stVehicle.LeftTopX = stPosition1.fX;
                stVehicle.LeftTopY = stPosition1.fY;
                stVehicle.RightBtmX = stPosition2.fX;
                stVehicle.RightBtmY = stPosition2.fY;
            }

            stVehicle.SubImageList.push_back(stImageInfo);
            stVehicles.push_back(stVehicle);
        }
    }

    int status = GAT1400::CGAT1400::instance()->uploadMotorvehicles(stVehicles);
    if (status) {
        dlog_debug("upload person faild %d", status);
    }
}

void CVehicleDetect::preProcessGat1400(
    const cv::Mat &image, const std::vector<VehicleDetect_NS::Result_S> &vecResult, int eventFlags)
{
    Network::Gat1400Client_S stConfig;
    GAT1400::CGAT1400::instance()->getGat1400Config(stConfig);
    if (!stConfig.enableGat1400) return;

    VehicleDetectEventType_S stEventStatus;
    convertMaskToType(eventFlags, stEventStatus);

    if (m_stAlgoParkingDetectionCfg.bEnable) {  // 停车侦测
        if (m_IllegalParkingUpload.handleAlarmState(stEventStatus.bIllegalParking)) {
            cv::Mat imageMat;
            cv::cvtColor(image, imageMat, cv::COLOR_YUV2BGR_NV12);
            pushVehiclesToGat1400(imageMat, vecResult);
        }
    }
}
#endif