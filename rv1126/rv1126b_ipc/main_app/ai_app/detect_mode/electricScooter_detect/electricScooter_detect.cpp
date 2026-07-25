/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-11-05 10:38:00
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-12-04 20:14:08
 * @FilePath: /1126/rv1126b_ipc/main_app/ai_app/algorithm_mode/algorithm/electricScooter_detect/electricScooter_detect.cpp
 * @Description: 电瓶车检测
 */

// #include "algo_stream_deal.h"
#include "StatisticsTimer.hpp"
#include "SaveImage.hpp"
#include "electricScooter_detect.hpp"

/* 数据队列 */
#define QUEUE_MAX (2)

CElectricScooterDetect::CElectricScooterDetect()
    : m_dateQueue(QUEUE_MAX)
{
    m_bRunning.store(true);
    m_thread = std::thread(&CElectricScooterDetect::run, this);
}

CElectricScooterDetect::~CElectricScooterDetect()
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
void CElectricScooterDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_stAlgoElectricScooterCfg.bEnable)
    {
        dlog_debug("ai_app:  电瓶车识别-开关未启用");
        return;
    }
	 
    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error(" 电瓶车识别-数据队列满了 [%d]" ,m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

float CElectricScooterDetect::sensitivityToConfidence(int sensitivity, float minConfidence, float maxConfidence) 
{
    int clampedSens = std::clamp(sensitivity, 1, 100);

    float confidence = maxConfidence - (clampedSens - 1) * (maxConfidence - minConfidence) / (100 - 1);

    confidence = std::clamp(confidence, minConfidence, maxConfidence);

    return confidence;
}

int CElectricScooterDetect::sensitivityToFrames(int sensitivity, int minFrames, int maxFrames) 
{
    if (sensitivity <= 0) return maxFrames;
    if (sensitivity >= 100) return minFrames;
    
    // 线性映射
    double ratio = (100.0 - sensitivity) / 100.0;
    int frames = minFrames + static_cast<int>(ratio * (maxFrames - minFrames));
    
    return frames;
}

bool CElectricScooterDetect::init()
{
    if (!m_pHandle)
    {
        ElectricScooterDetect_NS::InParam_S stInParam;
        stInParam.strModelPath = "/opt/cam/model/ElectricScooterDetect.json";
        stInParam.bDebug = false;

        m_pHandle = new ElectricScooterDetect_NS::CElectricScooterDetectV1_0(stInParam);
        if (m_pHandle)
        {
            if (m_pHandle->init())
            {
                dlog_debug("ai_app:  电瓶车识别算法初始化成功, %s", stInParam.strModelPath.c_str());
              
                return true;
            }
            else
            {
                delete m_pHandle;
                m_pHandle = nullptr;
                dlog_debug(" 电瓶车识别算法初始化失败");
            }
        }
    }
    return false;
}

bool CElectricScooterDetect::unInit()
{
    if (m_pHandle)
    {
        delete m_pHandle;
        m_pHandle = nullptr;
    }
    
    return true;
}

/* 事件转换函数 */
int CElectricScooterDetect::convertMaskToType( int eventFlags, ElectricScooterDetectEventType_S &stEventStatus)
{
    // dlog_debug("ai_app: 电瓶车检测事件 [%s]\n", std::bitset<4>(eventFlags).to_string().c_str());
    
    /* Event::Type::ELECTRIC_VEHICLE_IN_ELEVATOR */
    if (eventFlags & 0x01)
    {
        stEventStatus.bElectricScooterInElevator = true;
    }

    return 0;
}

/**
 * @brief 更新算法配置参数
 * @param stAlgoConfig 
 */
void CElectricScooterDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stAlgoElectricScooterCfg.bEnable = stAlgoConfig.nEnElectricVehicleInElevator;
	
	if(m_stAlgoElectricScooterCfg.bEnable)
	{
		Alarm::ElectricScooterDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
	}
	
}

void CElectricScooterDetect::setAlgoParamCfg(const Alarm::ElectricScooterDetection_S &stAlgoCfg,Event::Type_E enType)
{
    dlog_debug("ai_app: 设置电瓶车识别参数");
    m_stAlgoElectricScooterCfg = stAlgoCfg;
}

void CElectricScooterDetect::run()
{
    MediaData_S      stMediaData;   
    std::vector<ElectricScooterDetect_NS::Result_S> vecResult;

    while (m_bRunning.load())
    {
        if (!m_pHandle)
        {
            if (!init())
            {
                dlog_error("等待 电瓶车识别初始化");
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
      
        CStatisticsTimer runTime(" 电瓶车识别完整耗时");

        /* 送分析 */
        if (1)
        {
            frameRate(" 电瓶车识别-分析数据", 5);

            ElectricScooterDetect_NS::InData_S stInData {};
            ElectricScooterDetect_NS::OutData_S stOutData;
            
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
                if (access("/electricScooter_debugImage", F_OK) == 0)
                {
                    dlog_debug("============>debugImage");
                    cv::imwrite("/opt/algo/Image/electricScooter_algo.jpg", stInData.inMat);
                }

				if(m_stAlgoElectricScooterCfg.bEnable)
				{
					stInData.stParam.stElectricScooterParam.bEnable = true;
                    stInData.stParam.stElectricScooterParam.fConfidence = sensitivityToConfidence(m_stAlgoElectricScooterCfg.stRule.nSensitivity);
                    stInData.stParam.stElectricScooterParam.nDetectFrame = sensitivityToFrames(m_stAlgoElectricScooterCfg.stRule.nSensitivity);
                }

                /* 分析数据 */
                {
                    CStatisticsTimer runTime(" 电瓶车识别算法耗时");
                    m_pHandle->process(stInData, vecResult, &stOutData);

                    ElectricScooterDetectEventType_S stEventStatus;

                    convertMaskToType(stOutData.nType, stEventStatus);

                    if(m_stAlgoElectricScooterCfg.bEnable)
                    {
                        m_ElectricScooterStateMachine.handleAlarmState(stEventStatus.bElectricScooterInElevator, Event::Type_E::ELECTRIC_VEHICLE_IN_ELEVATOR);
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
            dlog_error("ai_app:  电瓶车识别-获取虚拟地址失败");
        }
    }
}