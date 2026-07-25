/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-11-24 14:39:18
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-12-09 14:22:44
 * @FilePath:/1126/rv1126b_ipc/main_app/ai_app/detect_mode/pet_recognition/pet_recognition.cpp
 * @Description: 宠物识别
 */

#include "pet_recognition.hpp"
#include "common_process.h"

/* 数据队列 */
#define QUEUE_MAX (2)

CPetRecognition::CPetRecognition()
    : m_dateQueue(QUEUE_MAX)
{
    /* 启动线程 */
    m_bRunning.store(true);
    m_thread = std::thread(&CPetRecognition::run, this);
}

CPetRecognition::~CPetRecognition()
{
    /* 通知线程停止 */
    m_bRunning.store(false);
    m_condition.notify_all();
    // note 调用 shutdown() 来唤醒可能阻塞在 pop() 的线程
    m_dateQueue.shutdown();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
    m_dateQueue.clear();
    unInit();
}

void CPetRecognition::recvMediaData(MediaData_S stMediaData)
{
    if (!m_stPetDetCfg.bEnable)
    {
        return;
    }

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("宠物识别-数据队列满了 [%d]", m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

void CPetRecognition::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stPetDetCfg.bEnable = stAlgoConfig.nEnPetRecognition;

    if (m_stPetDetCfg.bEnable)
    {
        Alarm::PetRecognition_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
}

void CPetRecognition::setAlgoParamCfg(const Alarm::PetRecognition_S &stAlgoCfg)
{
    dlog_debug("ai_app: 设置宠物识别参数");
    m_stPetDetCfg = stAlgoCfg;
    /* 转换区域坐标分辨率至算法分辨率 */
    m_stPetDetCfg.stRegion.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight);
}

bool CPetRecognition::init()
{
    if (!m_pPetDetHandle)
    {
        PetRecognition_NS::InParam_S stInParam;
        stInParam.strModelPath = "/opt/cam/model/PetRecognitionDetect.json";
        stInParam.bDebug = false;

        m_pPetDetHandle = new PetRecognition_NS::CPetRecognitionV1_0(stInParam);
        if (m_pPetDetHandle)
        {
            if (m_pPetDetHandle->init())
            {
                dlog_debug("ai_app:  宠物识别检测算法初始化成功, %s", stInParam.strModelPath.c_str());
              
                return true;
            }
            else
            {
                delete m_pPetDetHandle;
                m_pPetDetHandle = nullptr;
                dlog_debug(" 宠物识别检测算法初始化失败");
            }
        }
    }
    return false;
}

/* 反初始化 */
bool CPetRecognition::unInit()
{
    if (m_pPetDetHandle)
    {
        delete m_pPetDetHandle;
        m_pPetDetHandle = nullptr;
    }
    
    return true;
}

bool CPetRecognition::reboot()
{
    if(!unInit())
    {
        return false;
    }
    if(!init())
    {
        return false;
    }

    return true;
}

void CPetRecognition::run()
{
    pthread_setname_np(pthread_self(), "PetDetect");

    /* 媒体信息 */
    MediaData_S stMediaData;

    while (m_bRunning.load())
    {
        if (!m_pPetDetHandle)
        {
            /* 没有算法使能，不进行算法初始化 */
            if (!m_stPetDetCfg.bEnable)
            {
                sleep(1);
                continue;
            }

            if (!init())
            {
                dlog_error("等待宠物识别初始化");
                /* 延迟等待 1s */
                std::unique_lock<std::mutex> lock(m_mutex);
                m_condition.wait_for(lock, std::chrono::seconds(1),
                                     [this]
                                     {
                                         return !m_bRunning.load();
                                     });
            }
            continue;
        }

        /* 阻塞获取 */
        // if(!m_dateQueue.pop(stMediaData, TIMEOUT_1000_MS) || stMediaData.pVideoFrameInfo == nullptr)
        m_dateQueue.pop(stMediaData, -1);
        if (stMediaData.nSize == 0)
        {
            /* 数据为空 */
            continue;
        }

        CStatisticsTimer runTime(" 宠物识别检测完整耗时");

        /* 宠物识别 */
        if (stMediaData.pData)
        {
            frameRate(" 宠物识别检测-分析数据", 5);

            PetRecognition_NS::InData_S stInData {};
            std::vector<PetRecognition_NS::Result_S>  stResults;
            
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
                /* 宠物识别 */
                if (m_stPetDetCfg.bEnable)
                {
                    /* 宠物识别配置的灵敏度阈值 */
                    float fSensitivityThreshold = 1.0f - m_stPetDetCfg.nSensitivity / 100.0f;
                    stInData.stParam.stPetRecognitionParam.bEnable = true;
                    stInData.stParam.stPetRecognitionParam.fConfidence = fSensitivityThreshold;
                }

                /* 分析数据 */
                {
                    CStatisticsTimer runTime(" 宠物识别检测算法耗时");
                    m_pPetDetHandle->process(stInData, stResults);

                    // if (stResults.size() > 0)
                    // {
                        // double time = time_get_ms();
                        /* 宠物识别后处理函数 */
                        processPetRecognition(stResults);
                        // dlog_debug("宠物识别后处理函数耗时：%f", time_get_ms() - time);
                    // }
                }

            }
        }
    }
}

// info /*----------------------- 算法后处理 -----------------------*/
void CPetRecognition::processPetRecognition(std::vector<PetRecognition_NS::Result_S> &stResults)
{
    /* 是否报警 */
    bool bIsAlarm = false;
    /* OSD 动态分析显示数组 */
    std::vector<Common::RectInfo_S> vstRectInfo;

    /* 遍历出检测到的目标个数 */
    for (size_t i = 0; i < stResults.size(); i++)
    {
        Common::RectInfo_S stInfo;

        stInfo.nX1 = stResults.at(i).fX1;
        stInfo.nY1 = stResults.at(i).fY1;
        stInfo.nX2 = stResults.at(i).fX2;
        stInfo.nY2 = stResults.at(i).fY2;
        /* 判断识别结果是否在检测框内 */
        if (is_in_region(m_stPetDetCfg.stRegion, stInfo))
        {
            /* 宠物识别 动态分析 */
            if (m_stPetDetCfg.bDynamicAnalysisEnable)
            {
                vstRectInfo.emplace_back(stInfo);
            }
            bIsAlarm = true;
        }
    }

    /* 判断是否报警 */
    if (bIsAlarm)
    {
        /* 宠物识别 动态分析 */
        if (m_stPetDetCfg.bDynamicAnalysisEnable)
        {
            /* 发送结果至OSD模块，进行框选显示 */
            send_detectionResult_to_osd(m_nWidth, m_nHeight, vstRectInfo);
        }
    }
    m_petAlarmStateMachine.handleAlarmState(bIsAlarm, Event::Type_E::PET_RECOGNITION);

    return ;
}
