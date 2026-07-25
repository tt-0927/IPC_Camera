/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-11-05 10:38:00
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-04-17 16:23:06
 * @FilePath: /1126/rv1126b_ipc/main_app/ai_app/algorithm_mode/algorithm/licensePlateCognition_detect/licensePlateCognition_detect.cpp
 * @Description: 车牌识别
 */

#include "licensePlateCognition_detect.hpp"
// #include "algo_stream_deal.h"
#include "common_process.h"
#include "StatisticsTimer.hpp"
// #include "SaveImage.hpp"

/* 数据队列 */
#define QUEUE_MAX (2)

CLicensePlateCognitionDetect::CLicensePlateCognitionDetect()
    : m_dateQueue(QUEUE_MAX)
{
    m_bRunning.store(true);
    m_thread = std::thread(&CLicensePlateCognitionDetect::run, this);
}

CLicensePlateCognitionDetect::~CLicensePlateCognitionDetect()
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
void CLicensePlateCognitionDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_stAlgoLicensePlateCognitionCfg.bEnable)
    {
        dlog_debug("ai_app: 车牌识别检测-开关未启用");
        return;
    }
	 
    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("车牌识别检测-数据队列满了 [%d]" ,m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

float CLicensePlateCognitionDetect::sensitivityToConfidence(int sensitivity, float minConfidence, float maxConfidence) 
{
    int clampedSens = std::clamp(sensitivity, 1, 100);

    float confidence = maxConfidence - (clampedSens - 1) * (maxConfidence - minConfidence) / (100 - 1);

    confidence = std::clamp(confidence, minConfidence, maxConfidence);

    return confidence;
}

int CLicensePlateCognitionDetect::sensitivityToFrames(int sensitivity, int minFrames, int maxFrames) 
{
    if (sensitivity <= 0) return maxFrames;
    if (sensitivity >= 100) return minFrames;
    
    // 线性映射
    double ratio = (100.0 - sensitivity) / 100.0;
    int frames = minFrames + static_cast<int>(ratio * (maxFrames - minFrames));
    
    return frames;
}

bool CLicensePlateCognitionDetect::init()
{
    if (!m_pHandle)
    {
        LicensePlateCognition_NS::InParam_S stInParam;
        stInParam.strModelPath1 = "/opt/cam/model/LicensePlateDetect.json";
        stInParam.strModelPath2 = "/opt/cam/model/LicensePlateRec.json";
        // stInParam.strOriginalDataPath = "/mnt/licensePlateCognition_detect/";
        stInParam.bDebug = false;
        m_pHandle = new LicensePlateCognition_NS::CLicensePlateCognitionV1_0(stInParam);  
        
        if (m_pHandle)
        {
            if (m_pHandle->init())
            {
                dlog_debug("ai_app: 车牌检测算法初始化成功, %s", stInParam.strModelPath1.c_str());
                dlog_debug("ai_app: 车牌号识别检测算法初始化成功, %s", stInParam.strModelPath2.c_str());
              
                return true;
            }
            else
            {
                delete m_pHandle;
                m_pHandle = nullptr;
                dlog_debug("车牌识别检测算法初始化失败");
            }
        }
    }
    return false;
}

bool CLicensePlateCognitionDetect::unInit()
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
void CLicensePlateCognitionDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stAlgoLicensePlateCognitionCfg.bEnable = stAlgoConfig.nPlateNumber;
	if(m_stAlgoLicensePlateCognitionCfg.bEnable)
	{
		Alarm::LicensePlateCognitionDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
	}
    return;
}

void CLicensePlateCognitionDetect::setAlgoParamCfg(const Alarm::LicensePlateCognitionDetection_S &stAlgoCfg,Event::Type_E enType)
{
    dlog_debug("ai_app: 设置车牌识别检测参数");
    m_stAlgoLicensePlateCognitionCfg = stAlgoCfg;
    return;
}


void CLicensePlateCognitionDetect::run()
{
    MediaData_S      stMediaData;   
    // std::vector<LicensePlateCognition_NS::Result_S> vecResult;
    std::vector<LicensePlateCognition_NS::Result_S> vOutData;

    while (m_bRunning.load())
    {
        if (!m_pHandle)
        {
            if (!init())
            {
                dlog_error("等待车牌识别检测初始化");
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
      
        CStatisticsTimer runTime("车牌识别检测完整耗时");

        /* 送分析 */
        if (1)
        {
            frameRate("车牌识别检测-分析数据", 5);

            LicensePlateCognition_NS::InData_S stInData {};
            std::vector<LicensePlateCognition_NS::Result_S> vOutData;
            
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
                if (access("/LicensePlateCognition_debugImage", F_OK) == 0)
                {
                    dlog_debug("============>debugImage");
                    cv::imwrite("/opt/algo/Image/test_algo.jpg", stInData.inMat);
                }

				if(m_stAlgoLicensePlateCognitionCfg.bEnable)
				{
                    float sens = m_stAlgoLicensePlateCognitionCfg.stRule.nSensitivity;   // 1~100
                    // 把 0~100 映射到 5~95，除以 100 得到 0.05~0.95
                    float f = 5.0f + sens * 0.9f;      //  (5, 95]
                    stInData.stParam.fBoxThreshold = 1.0f - f / 100.0f;   // (0.05, 0.95]
                    dlog_debug("fConfidence = %f", stInData.stParam.fBoxThreshold);
                }
                
                /* 分析数据 */
                {
                    CStatisticsTimer runTime("车牌识别检测算法耗时");
                    m_pHandle->process(stInData, vOutData);
                    bool bIsAlarm = false;
                    if(vOutData.size() != 0)
                    {
                        dlog_debug("ai_app: 车牌识别检测报警触发");
                        bIsAlarm = true;
                    }
                    /* 车牌识别动态分析 */
                    dynamicAnalysis(vOutData); 
                    m_LicensePlateStateMachine.handleAlarmState(bIsAlarm, Event::Type_E::PLATE_NUMBER);
                }
            }
            else
            {
                dlog_error("ai_app: 图片数据为空");
            }
        }
        else
        {
            dlog_error("ai_app: 车牌识别检测-获取虚拟地址失败");
        }
    }
}

int CLicensePlateCognitionDetect::dynamicAnalysis(const std::vector<LicensePlateCognition_NS::Result_S> &vecResult)
{
    std::vector<Common::RectInfo_S> vstRectInfo;

    for (auto &stResult : vecResult)
    {
        Common::RectInfo_S stRectInfo;
        stRectInfo.nX1 = (int)stResult.fX;
        stRectInfo.nY1 = (int)stResult.fY;
        stRectInfo.nX2 = (int)(stResult.fX + stResult.fWidth);
        stRectInfo.nY2 = (int)(stResult.fY + stResult.fHeight);
        vstRectInfo.push_back(stRectInfo);
    }
    if(vstRectInfo.size())
    {
        send_detectionResult_to_osd(m_nWidth, m_nHeight, vstRectInfo);
    }
    return 0;
}