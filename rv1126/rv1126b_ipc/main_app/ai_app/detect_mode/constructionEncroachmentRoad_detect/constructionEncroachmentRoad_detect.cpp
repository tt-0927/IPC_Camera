/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-11-05 10:38:00
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-12-04 20:51:39
 * @FilePath: /1126/rv1126b_ipc/main_app/ai_app/algorithm_mode/algorithm/constructionEncroachmentRoad_detect/constructionEncroachmentRoad_detect.cpp
 * @Description: 施工占道检测
 */

#include "constructionEncroachmentRoad_detect.hpp"
#include "StatisticsTimer.hpp"

/* 数据队列 */
#define QUEUE_MAX (2)

CConstructionEncroachmentRoadDetect::CConstructionEncroachmentRoadDetect()
    : m_dateQueue(QUEUE_MAX)
{
    m_bRunning.store(true);
    m_thread = std::thread(&CConstructionEncroachmentRoadDetect::run, this);
}

CConstructionEncroachmentRoadDetect::~CConstructionEncroachmentRoadDetect()
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
void CConstructionEncroachmentRoadDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_stAlgoConstructionEncroachmentRoadCfg.bEnable)
    {
        dlog_debug("ai_app: 施工占道检测-开关未启用");
        return;
    }
	 
    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("施工占道检测-数据队列满了 [%d]" ,m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

float CConstructionEncroachmentRoadDetect::sensitivityToConfidence(int sensitivity, float minConfidence, float maxConfidence) 
{
    int clampedSens = std::clamp(sensitivity, 1, 100);

    float confidence = maxConfidence - (clampedSens - 1) * (maxConfidence - minConfidence) / (100 - 1);

    confidence = std::clamp(confidence, minConfidence, maxConfidence);

    return confidence;
}

int CConstructionEncroachmentRoadDetect::sensitivityToFrames(int sensitivity, int minFrames, int maxFrames) 
{
    if (sensitivity <= 0) return maxFrames;
    if (sensitivity >= 100) return minFrames;
    
    // 线性映射
    double ratio = (100.0 - sensitivity) / 100.0;
    int frames = minFrames + static_cast<int>(ratio * (maxFrames - minFrames));
    
    return frames;
}

bool CConstructionEncroachmentRoadDetect::init()
{
    if (!m_pHandle)
    {
        ConstructionEncroachmentRoadDetect_NS::InParam_S stInParam;
        stInParam.strModelPath = "/opt/cam/model/ConstructionDetect.json";
        stInParam.bDebug = false;
        m_pHandle = new ConstructionEncroachmentRoadDetect_NS::ConstructionEncroachmentRoadDetectV1_0(stInParam);
        
        if (m_pHandle)
        {
            if (m_pHandle->init())
            {
                dlog_debug("ai_app: 施工占道检测算法初始化成功, %s", stInParam.strModelPath.c_str());
              
                return true;
            }
            else
            {
                delete m_pHandle;
                m_pHandle = nullptr;
                dlog_debug("施工占道检测算法初始化失败");
            }
        }
    }
    return false;
}

bool CConstructionEncroachmentRoadDetect::unInit()
{
    if (m_pHandle)
    {
        delete m_pHandle;
        m_pHandle = nullptr;
    }
    
    return true;
}

/* 事件转换函数 */
int CConstructionEncroachmentRoadDetect::convertMaskToType( int eventFlags)
{
    
    { if (eventFlags & 0x02) return 33; /* Event::Type::SMOKE_FIRE */ }
    { if (eventFlags & 0x04) return 34; /* Event::Type::OPEN_FLAME */ }

    return -1;
}

/**
 * @brief 更新算法配置参数
 * @param stAlgoConfig 
 */
void CConstructionEncroachmentRoadDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stAlgoConstructionEncroachmentRoadCfg.bEnable = stAlgoConfig.nEnConstructionOccupyRoad;
	if(m_stAlgoConstructionEncroachmentRoadCfg.bEnable)
	{
		Alarm::ConstructionEncroachmentRoadDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
	}
    return;
}

void CConstructionEncroachmentRoadDetect::setAlgoParamCfg(const Alarm::ConstructionEncroachmentRoadDetection_S &stAlgoCfg,Event::Type_E enType)
{
    dlog_debug("ai_app: 设置施工占道检测参数");
    m_stAlgoConstructionEncroachmentRoadCfg = stAlgoCfg;
    return;
}


void CConstructionEncroachmentRoadDetect::run()
{
    MediaData_S      stMediaData;   
    std::vector<ConstructionEncroachmentRoadDetect_NS::Result_S> vecResult;

    while (m_bRunning.load())
    {
        if (!m_pHandle)
        {
            if (!init())
            {
                dlog_error("等待施工占道检测初始化");
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
      
        CStatisticsTimer runTime("施工占道检测完整耗时");

        /* 送分析 */
        if (1)
        {
            frameRate("施工占道检测-分析数据", 5);

            ConstructionEncroachmentRoadDetect_NS::InData_S stInData {};
            ConstructionEncroachmentRoadDetect_NS::OutData_S stOutData;
            
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

				if(m_stAlgoConstructionEncroachmentRoadCfg.bEnable)
				{
					stInData.stParam.stConstructionEncroachmentRoadParam.bEnable = true;
					stInData.stParam.stConstructionEncroachmentRoadParam.nDetectFrame = sensitivityToFrames(m_stAlgoConstructionEncroachmentRoadCfg.stRule.nSensitivity);
                    stInData.stParam.stConstructionEncroachmentRoadParam.fConfidence = sensitivityToConfidence(m_stAlgoConstructionEncroachmentRoadCfg.stRule.nSensitivity);
                }

                if (access("/constructionEncroachmentRoad_debugImage", F_OK) == 0)
                {
                    dlog_debug("============>debugImage");
                    cv::imwrite("/opt/algo/Image/constructionEncroachmentRoad_algo.jpg", stInData.inMat);
                    dlog_debug("fConfidence = %f, nDetectFrame = %d", stInData.stParam.stConstructionEncroachmentRoadParam.fConfidence, stInData.stParam.stConstructionEncroachmentRoadParam.nDetectFrame);
                }
                
                /* 分析数据 */
                {
                    CStatisticsTimer runTime("施工占道检测算法耗时");
                    m_pHandle->process(stInData, vecResult, &stOutData);
                    
                    if(m_stAlgoConstructionEncroachmentRoadCfg.bEnable)
                    {
                        m_ConstructionEncroachmentRoadStateMachine.handleAlarmState(stOutData.bConstructionEncroachmentRoad, Event::Type_E::CONSTRUCTION_OCCUPY_ROAD);
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
            dlog_error("ai_app: 施工占道检测-获取虚拟地址失败");
        }
    }
}