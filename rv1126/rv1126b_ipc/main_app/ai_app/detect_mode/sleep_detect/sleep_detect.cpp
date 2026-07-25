/**
 * @file sleep_detect.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-13
 * 
 * @brief 睡岗识别相关
 */

#include "sleep_detect.hpp"
#include "algo_stream_deal.h"
#include "StatisticsTimer.hpp"
#include "SaveImage.hpp"
#include "SleepOnDutyDetectV1_0.hpp"

/* 数据队列 */
#define QUEUE_MAX (2)

CSleepDetect::CSleepDetect()
    : m_dateQueue(QUEUE_MAX)
{
    m_bRunning.store(true);
    m_thread = std::thread(&CSleepDetect::run, this);
}

CSleepDetect::~CSleepDetect()
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
void CSleepDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_stAlgoSleepOnDutyCfg.bEnable)
    {
        dlog_debug("ai_app:  睡岗识别-开关未启用");
        return;
    }
	 
    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error(" 睡岗识别-数据队列满了 [%d]" ,m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

float CSleepDetect::sensitivityToConfidence(int sensitivity, float minConfidence, float maxConfidence) 
{
    int clampedSens = std::clamp(sensitivity, 1, 100);

    float confidence = maxConfidence - (clampedSens - 1) * (maxConfidence - minConfidence) / (100 - 1);

    confidence = std::clamp(confidence, minConfidence, maxConfidence);

    return confidence;
}

int CSleepDetect::sensitivityToFrames(int sensitivity, int minFrames, int maxFrames) 
{
    if (sensitivity <= 0) return maxFrames;
    if (sensitivity >= 100) return minFrames;
    
    // 线性映射
    double ratio = (100.0 - sensitivity) / 100.0;
    int frames = minFrames + static_cast<int>(ratio * (maxFrames - minFrames));
    
    return frames;
}

bool CSleepDetect::init()
{
    if (!m_pHandle)
    {
        SleepOnDutyDetect_NS::InParam_S stInParam;
        stInParam.strModelPath = "/opt/cam/model/SleepDetect.json";
        stInParam.bDebug = false;

        m_pHandle = new SleepOnDutyDetect_NS::CSleepOnDutyDetectV1_0(stInParam);
        if (m_pHandle)
        {
            if (m_pHandle->init())
            {
                dlog_debug("ai_app:  睡岗识别算法初始化成功, %s", stInParam.strModelPath.c_str());
              
                return true;
            }
            else
            {
                delete m_pHandle;
                m_pHandle = nullptr;
                dlog_debug(" 睡岗识别算法初始化失败");
            }
        }
    }
    return false;
}

bool CSleepDetect::unInit()
{
    if (m_pHandle)
    {
        delete m_pHandle;
        m_pHandle = nullptr;
    }
    
    return true;
}

/* 事件转换函数 */
int CSleepDetect::convertMaskToType( int eventFlags)
{
    
    { if (eventFlags & 0x02) return 25; /* Event::Type::SLEEP_ON_DUTY */ }

    return -1;
}

/**
 * @brief 更新算法配置参数
 * @param stAlgoConfig 
 */
void CSleepDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stAlgoSleepOnDutyCfg.bEnable = stAlgoConfig.nEnSleepOnDuty;
	
	if(m_stAlgoSleepOnDutyCfg.bEnable)
	{
		Alarm::SleepOnDutyDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
	}
	
}

void CSleepDetect::setAlgoParamCfg(const Alarm::SleepOnDutyDetection_S &stAlgoCfg,Event::Type_E enType)
{
    dlog_debug("ai_app: 设置睡岗识别参数");
    m_stAlgoSleepOnDutyCfg = stAlgoCfg;
}

void CSleepDetect::run()
{
    MediaData_S      stMediaData;   
    std::vector<SleepOnDutyDetect_NS::Result_S> vecResult;

    while (m_bRunning.load())
    {
        if (!m_pHandle)
        {
            if (!init())
            {
                dlog_error("等待 睡岗识别初始化");
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
      
        CStatisticsTimer runTime(" 睡岗识别完整耗时");

        /* 送分析 */
        if (1)
        {
            frameRate(" 睡岗识别-分析数据", 5);

            SleepOnDutyDetect_NS::InData_S stInData {};
            SleepOnDutyDetect_NS::OutData_S stOutData;
            
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
                if (access("/sleep_debugImage", F_OK) == 0)
                {
                    dlog_debug("============>debugImage");
                    cv::imwrite("/opt/algo/Image/test_algo.jpg", stInData.inMat);
                }

				if(m_stAlgoSleepOnDutyCfg.bEnable)
				{
					stInData.stParam.stSleepOnDutyParam.bEnable = true;
                    stInData.stParam.stSleepOnDutyParam.fConfidence = sensitivityToConfidence(m_stAlgoSleepOnDutyCfg.stRule.nSensitivity);
                    stInData.stParam.stSleepOnDutyParam.nDetectFrame = sensitivityToFrames(m_stAlgoSleepOnDutyCfg.stRule.nSensitivity);
                }

                /* 分析数据 */
                {
                    CStatisticsTimer runTime(" 睡岗识别算法耗时");
                    m_pHandle->process(stInData, vecResult, &stOutData);

                    if (stOutData.validResult)
                    {
                        for (const auto& result : vecResult)
                        {
                            int nType = convertMaskToType(stOutData.nType);
                            if (nType != -1 )
                            {
                                /* 上报事件 */
                                dlog_debug("ai_app:  睡岗识别报警触发 类型[%d]",nType);
                                CEventLinkage::instance()->handleEvent(static_cast<Event::Type_E>(nType), false);
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
            dlog_error("ai_app:  睡岗识别-获取虚拟地址失败");
        }
    }
}