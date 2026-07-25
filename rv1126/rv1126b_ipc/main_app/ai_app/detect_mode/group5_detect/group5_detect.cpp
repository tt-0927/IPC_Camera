/*
 * @Author: lianghy lianghy@kfb.cn
 * @Date: 2026-01-08 19:22:49
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-01-29 11:40:12
 * @FilePath: /1126/rv1126b_ipc/main_app/ai_app/detect_mode/group3_detect/group5_detect.cpp
 * @Description: metalFence(金属栅栏)、ConeTank(锥形桶)、CrashBarrels(防撞桶)、fence(防护栏)
 */
 
#include "group5_detect.hpp"
#include "algo_stream_deal.h"
#include "StatisticsTimer.hpp"
#include "SaveImage.hpp"

/* 数据队列 */
#define QUEUE_MAX (2)

CGroup5Detect::CGroup5Detect()
    : m_dateQueue(QUEUE_MAX)
{
    m_bRunning.store(true);
    m_thread = std::thread(&CGroup5Detect::run, this);
}

CGroup5Detect::~CGroup5Detect()
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
void CGroup5Detect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_stAlgoHoleProtectionBarCfg.bEnable && !m_stAlgoConstructionEncroachmentRoadCfg.bEnable)
    {
        dlog_debug("ai_app:  模型组合5识别-开关未启用");
        return;
    }
	 
    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error(" 模型组合5识别-数据队列满了 [%d]" ,m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

float CGroup5Detect::sensitivityToConfidence(int sensitivity, float minConfidence, float maxConfidence) 
{
    int clampedSens = std::clamp(sensitivity, 1, 100);

    float confidence = maxConfidence - (clampedSens - 1) * (maxConfidence - minConfidence) / (100 - 1);

    confidence = std::clamp(confidence, minConfidence, maxConfidence);

    return confidence;
}

int CGroup5Detect::sensitivityToFrames(int sensitivity, int minFrames, int maxFrames) 
{
    if (sensitivity <= 0) return maxFrames;
    if (sensitivity >= 100) return minFrames;
    
    // 线性映射
    double ratio = (100.0 - sensitivity) / 100.0;
    int frames = minFrames + static_cast<int>(ratio * (maxFrames - minFrames));
    
    return frames;
}

bool CGroup5Detect::init()
{
    if (!m_pHandle)
    {
        Group5Detect_NS::InParam_S stInParam;
        stInParam.strModelPath = "/opt/cam/model/group5.json";
        stInParam.bDebug = false;

        m_pHandle = new Group5Detect_NS::CGroup5DetectV1_0(stInParam);
        if (m_pHandle)
        {
            if (m_pHandle->init())
            {
                dlog_debug("ai_app:  模型组合5识别算法初始化成功, %s", stInParam.strModelPath.c_str());
              
                return true;
            }
            else
            {
                delete m_pHandle;
                m_pHandle = nullptr;
                dlog_debug(" 模型组合5识别算法初始化失败");
            }
        }
    }
    return false;
}

bool CGroup5Detect::unInit()
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
void CGroup5Detect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stAlgoHoleProtectionBarCfg.bEnable = stAlgoConfig.nEnHoleProtectionBar;
    m_stAlgoConstructionEncroachmentRoadCfg.bEnable = stAlgoConfig.nEnConstructionOccupyRoad;
	
	if(m_stAlgoHoleProtectionBarCfg.bEnable)
	{
		Alarm::HoleProtectionBarDection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
	}
    
	if(m_stAlgoConstructionEncroachmentRoadCfg.bEnable)
	{
		Alarm::ConstructionEncroachmentRoadDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
	}

    return ;
}

void CGroup5Detect::setAlgoParamCfg(const Alarm::HoleProtectionBarDection_S &stAlgoCfg,Event::Type_E enType)
{
    dlog_debug("ai_app: 设置洞口防护栏检测参数");
    m_stAlgoHoleProtectionBarCfg = stAlgoCfg;
}

void CGroup5Detect::setAlgoParamCfg(const Alarm::ConstructionEncroachmentRoadDetection_S &stAlgoCfg,Event::Type_E enType)
{
    dlog_debug("ai_app: 设置施工占道检测参数");
    m_stAlgoConstructionEncroachmentRoadCfg = stAlgoCfg;
    return;
}


void CGroup5Detect::run()
{
    MediaData_S      stMediaData;   
    std::vector<Group5Detect_NS::Result_S> vecResult;

    while (m_bRunning.load())
    {
        if (!m_pHandle)
        {
            if (!init())
            {
                dlog_error("等待 模型组合5识别初始化");
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
      
        CStatisticsTimer runTime(" 模型组合5识别完整耗时");

        /* 送分析 */
        if (stMediaData.pData)
        {
            frameRate(" 模型组合5识别-分析数据", 5);

            Group5Detect_NS::InData_S stInData {};
            Group5Detect_NS::OutData_S stOutData;
            
             cv::Mat i420Mat(
                stMediaData.stMediaParam.nVideoHeight * 3/2,
                stMediaData.stMediaParam.nVideoWidth,
                CV_8UC1,
                stMediaData.pData.get()
            );

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
                if (access("group5Detect_debugImage", F_OK) == 0)
                {
                    dlog_debug("============>debugImage");
                    cv::imwrite("/opt/algo/Image/group5Detect_debugImage.jpg", stInData.inMat);
                }
                
                /* 洞口防护栏识别 */
				if(m_stAlgoHoleProtectionBarCfg.bEnable)
				{
                    stInData.stParam.stHoleProtectionBarDetectParam.bEnable = true;
                    stInData.stParam.stHoleProtectionBarDetectParam.fConfidence = sensitivityToConfidence(m_stAlgoHoleProtectionBarCfg.stRule.nSensitivity);
                    stInData.stParam.stHoleProtectionBarDetectParam.nDetectFrame = sensitivityToFrames(m_stAlgoHoleProtectionBarCfg.stRule.nSensitivity);
                }

                /* 施工占道识别 */
                if(m_stAlgoConstructionEncroachmentRoadCfg.bEnable)
                {
                    stInData.stParam.stConstructionEncroachmentRoad.bEnable = true;
                    stInData.stParam.stConstructionEncroachmentRoad.fConfidence = sensitivityToConfidence(m_stAlgoConstructionEncroachmentRoadCfg.stRule.nSensitivity);
                    stInData.stParam.stConstructionEncroachmentRoad.nDetectFrame = sensitivityToFrames(m_stAlgoConstructionEncroachmentRoadCfg.stRule.nSensitivity);
                }

                /* 分析数据 */
                {
                    CStatisticsTimer runTime(" 模型组合5识别算法耗时");
                    m_pHandle->process(stInData, vecResult, &stOutData);
                    /* 检测后处理 */
                    processGroup5Detect(stOutData);
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
            dlog_error("ai_app:  模型组合5识别-获取虚拟地址失败");
        }
    }
}

int CGroup5Detect::dynamicAnalysis(const std::vector<Group5Detect_NS::Result_S> &vecResult)
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

void CGroup5Detect::processGroup5Detect(const Group5Detect_NS::OutData_S &stOutData) 
{
    if(m_stAlgoHoleProtectionBarCfg.bEnable)
    {
        m_HoleProtectionBarStateMachine.handleAlarmState(stOutData.bHoleProtectionBar, Event::Type_E::HOLE_PROTECTION_BAR);
    }
    if(m_stAlgoConstructionEncroachmentRoadCfg.bEnable)
    {
        m_ConstructionEncroachmentRoadStateMachine.handleAlarmState(stOutData.bConstructionEncroachmentRoad, Event::Type_E::CONSTRUCTION_OCCUPY_ROAD);
    }

    return ;
}
