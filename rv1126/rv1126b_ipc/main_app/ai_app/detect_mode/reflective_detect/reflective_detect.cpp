/**
 * @file reflective_detect.cpp
 * @author tianl (tianl@kfb.cn)
 * @date 2025-11-13
 * 
 * @brief 反光衣识别
 */

#include "reflective_detect.hpp"
#include "algo_stream_deal.h"
#include "StatisticsTimer.hpp"
#include "SaveImage.hpp"

/* 数据队列 */
#define QUEUE_MAX (2)

CReflectiveDetect::CReflectiveDetect()
    : m_dateQueue(QUEUE_MAX)
{
    m_bRunning.store(true);
    m_thread = std::thread(&CReflectiveDetect::run, this);
}

CReflectiveDetect::~CReflectiveDetect()
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
void CReflectiveDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_stAlgoReflectiveClothingCfg.bEnable)
    {
        dlog_debug("ai_app:  反光衣识别-开关未启用");
        return;
    }
	 
    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error(" 反光衣识别-数据队列满了 [%d]" ,m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

float CReflectiveDetect::sensitivityToConfidence(int sensitivity, float minConfidence, float maxConfidence) 
{
    int clampedSens = std::clamp(sensitivity, 1, 100);

    float confidence = maxConfidence - (clampedSens - 1) * (maxConfidence - minConfidence) / (100 - 1);

    confidence = std::clamp(confidence, minConfidence, maxConfidence);

    return confidence;
}

int CReflectiveDetect::sensitivityToFrames(int sensitivity, int minFrames, int maxFrames) 
{
    if (sensitivity <= 0) return maxFrames;
    if (sensitivity >= 100) return minFrames;
    
    // 线性映射
    double ratio = (100.0 - sensitivity) / 100.0;
    int frames = minFrames + static_cast<int>(ratio * (maxFrames - minFrames));
    
    return frames;
}

bool CReflectiveDetect::init()
{
    if (!m_pHandle)
    {
        ReflectiveDetect_NS::InParam_S stInParam;
        stInParam.strModelPath = "/opt/cam/model/ReflectiveDetect.json";
        stInParam.bDebug = false;

        m_pHandle = new ReflectiveDetect_NS::CReflectiveDetectV1_0(stInParam);
        if (m_pHandle)
        {
            if (m_pHandle->init())
            {
                dlog_debug("ai_app:  反光衣识别算法初始化成功, %s", stInParam.strModelPath.c_str());
              
                return true;
            }
            else
            {
                delete m_pHandle;
                m_pHandle = nullptr;
                dlog_debug(" 反光衣识别算法初始化失败");
            }
        }
    }
    return false;
}

bool CReflectiveDetect::unInit()
{
    if (m_pHandle)
    {
        delete m_pHandle;
        m_pHandle = nullptr;
    }
    
    return true;
}

/* 事件转换函数 */
int CReflectiveDetect::convertMaskToType( int eventFlags)
{
    
    { if (eventFlags & 0x02) return 41; /* Event::Type::REFLECTIVE_CLOTHING */ }

    return -1;
}

/**
 * @brief 更新算法配置参数
 * @param stAlgoConfig 
 */
void CReflectiveDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stAlgoReflectiveClothingCfg.bEnable = stAlgoConfig.nEnReflectiveClothing;
	
	if(m_stAlgoReflectiveClothingCfg.bEnable)
	{
		Alarm::ReflectiveClothingDection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
	}
	
}

void CReflectiveDetect::setAlgoParamCfg(const Alarm::ReflectiveClothingDection_S &stAlgoCfg,Event::Type_E enType)
{
    dlog_debug("ai_app: 设置反光衣识别参数");
    m_stAlgoReflectiveClothingCfg = stAlgoCfg;
}

void CReflectiveDetect::run()
{
    MediaData_S      stMediaData;   
    std::vector<ReflectiveDetect_NS::Result_S> vecResult;

    while (m_bRunning.load())
    {
        if (!m_pHandle)
        {
            if (!init())
            {
                dlog_error("等待 反光衣识别初始化");
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
      
        CStatisticsTimer runTime(" 反光衣识别完整耗时");

        /* 送分析 */
        if (1)
        {
            frameRate(" 反光衣识别-分析数据", 5);

            ReflectiveDetect_NS::InData_S stInData {};
            ReflectiveDetect_NS::OutData_S stOutData;
            
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
                if (access("Reflective_debugImage", F_OK) == 0)
                {
                    dlog_debug("============>debugImage");
                    cv::imwrite("/opt/algo/Image/test_algo.jpg", stInData.inMat);
                }

				if(m_stAlgoReflectiveClothingCfg.bEnable)
				{
					stInData.stParam.stReflectiveyParam.bEnable = true;
                    stInData.stParam.stReflectiveyParam.fConfidence = sensitivityToConfidence(m_stAlgoReflectiveClothingCfg.stRule.nSensitivity);
                    stInData.stParam.stReflectiveyParam.nDetectFrame = sensitivityToFrames(m_stAlgoReflectiveClothingCfg.stRule.nSensitivity);
                }

                /* 分析数据 */
                {
                    CStatisticsTimer runTime(" 反光衣识别算法耗时");
                    m_pHandle->process(stInData, vecResult, &stOutData);

                    if (stOutData.validResult)
                    {
                        for (const auto& result : vecResult)
                        {
                            int nType = convertMaskToType(stOutData.nType);
                            if (nType != -1 )
                            {
                                /* 上报事件 */
                                dlog_debug("ai_app:  反光衣识别报警触发 类型[%d]",nType);
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
            dlog_error("ai_app:  反光衣识别-获取虚拟地址失败");
        }
    }
}