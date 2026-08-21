/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-11-05 10:38:00
 * @LastEditors: lianghy lianghy@kfb.cn
 * @LastEditTime: 2026-04-22 16:30:50
 * @FilePath: /1126/rv1126b_ipc/main_app/ai_app/algorithm_mode/algorithm/scene_change_detect/scene_change_detect.cpp
 * @Description: 场景变更
 */

#include "scene_change_detect.hpp"
#include "event_configure.h"

/* 数据队列 */
#define QUEUE_MAX (2)

SceneChangeDetect::SceneChangeDetect()
    : m_dateQueue(QUEUE_MAX)
{
    /* 启动线程 */
    m_bRunning.store(true);
    m_thread = std::thread(&SceneChangeDetect::run, this);
}

SceneChangeDetect::~SceneChangeDetect()
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

/* 接受媒体数据 */
void SceneChangeDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_stSceneChangeDetCfg.bEnable || !m_pSceneChangeDetHandle)
    {
        return;
    }

    m_nChannelId = stMediaData.stMediaParam.nChannel;

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("场景变更-数据队列满了 [%d]", m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }

    return ;
}

/**
 * @brief 更新算法配置参数
 * @param stAlgoConfig 
 */
void SceneChangeDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stSceneChangeDetCfg.bEnable = stAlgoConfig.nEnSceneChange;

    if (m_stSceneChangeDetCfg.bEnable)
    {
        Alarm::SceneChange_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
}

void SceneChangeDetect::setAlgoParamCfg(const Alarm::SceneChange_S &stAlgoCfg)
{
    dlog_debug("ai_app: 设置场景变更参数");
    m_stSceneChangeDetCfg = stAlgoCfg;

    m_duration = time(NULL);
    m_LastFrameFrameTime = time(NULL);
    return ;
}

/* 初始化 */
bool SceneChangeDetect::init()
{
    if (!m_pSceneChangeDetHandle)
    {
        MoveDetect_NS::InParam_S stInParam;
        stInParam.bDebug = false;
        // stInParam.strAnalyzeDataPath = m_strAnalyzeDataPath + "record";
        // stInParam.strOriginalDataPath = "/root/OriginalImage";
        m_pSceneChangeDetHandle = new MoveDetect_NS::CMoveDetectV2_0(stInParam);
        if (m_pSceneChangeDetHandle)
        { 
            m_pSceneChangeDetHandle->set_resolution(m_nWidth, m_nHeight);
            if (m_pSceneChangeDetHandle->init())
            {
                dlog_info("场景变更算法初始化成功");
                return true;
            }
            else
            {
                delete m_pSceneChangeDetHandle;
                m_pSceneChangeDetHandle = nullptr;
                dlog_error("场景变更算法初始化失败");
            }
        }
    }
    return false;
}

/* 反初始化 */
bool SceneChangeDetect::unInit()
{
    if (m_pSceneChangeDetHandle)
    {
        delete m_pSceneChangeDetHandle;
        m_pSceneChangeDetHandle = nullptr;
    }
    return true;
}

bool SceneChangeDetect::reboot()
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

/* 线程函数 */
void SceneChangeDetect::run()
{
    MediaData_S      stMediaData;

    while (m_bRunning.load())
    {
        if (!m_pSceneChangeDetHandle)
        {
            if (!init())
            {
                dlog_error("等待场景变更初始化");
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
            continue;
        }
        // CStatisticsTimer runTime("场景变更完整耗时");

        if (stMediaData.pData)
        {
            frameRate("场景变更-分析数据", 5);
            MoveDetect_NS::InData_S stInData;
            std::vector<std::vector<int>> stOutData;

            int w = stMediaData.stMediaParam.nVideoWidth;
            int h = stMediaData.stMediaParam.nVideoHeight;
            uint8_t* pData = reinterpret_cast<uint8_t*>(stMediaData.pData.get());

            /* 单平面 NV12 */
            cv::Mat nv12(h * 3 / 2, w, CV_8UC1, pData);
            /* NV12→RGB */
            cv::cvtColor(nv12, stInData.inMat, cv::COLOR_YUV2RGB_NV12);

            m_stInDataMat = stInData.inMat.clone();

            // cv::rotate(stInData.inMat, stInData.inMat, cv::ROTATE_180);
            
            // int time1 = time(NULL);
            // std::string filename = "/mnt/scene_change_detect/yuanroi_nv12_to_bgr_" + std::to_string(time1) + ".jpg";
            // cv::imwrite(filename, stInData.inMat);   // 保存为 JPG

            if (!stInData.inMat.empty())
            {
                /* 分析数据 */
                {  
                    std::unique_lock<std::mutex> lock(m_mutex);
                    if (m_pSceneChangeDetHandle->LastFrameCache.empty())
                    {
                        m_pSceneChangeDetHandle->LastFrameCache.create(stInData.inMat.rows, 
                                stInData.inMat.cols, stInData.inMat.type());
                        
                        stInData.inMat.copyTo(m_pSceneChangeDetHandle->LastFrameCache);                        
                    }
                    {
                        // CStatisticsTimer runTime("场景变更 分析耗时");
                        bool bIsAlarm = false;
                        if (!m_pSceneChangeDetHandle->process(stInData, stOutData))
                        {
                            dlog_error("场景变更处理失败");
                        }
                        else 
                        {
                            bIsAlarm = sceneChangeDetectProcess(stOutData);
                        }

                        if((time(NULL) - m_LastFrameFrameTime > m_UpdateLastFrameDuration) || bIsAlarm)
                        {
                            m_LastFrameFrameTime = time(NULL);
                            stInData.inMat.copyTo(m_pSceneChangeDetHandle->LastFrameCache);
                        }
                        
                        // m_pSceneChangeDetHandle->LastFrameCache.release();
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
            dlog_error("ai_app: 场景变更-原始数据帧为空");
        }
    }
}

bool SceneChangeDetect::sceneChangeDetectProcess(std::vector<std::vector<int>> &vstRectsInfo)
{
    /* 处理检测到的矩形区域 */
    /* 打印输出数据 */
    if (!access("testPrint", F_OK))
    {
        printResult(vstRectsInfo);
    }
    /* 是否报警 */
    bool bIsAlarm = false;
    // int nMaxAreaIndex = 0;

    int nMaxArea = 0;
    int nArea = 0;
    for(unsigned int i = 0; i < vstRectsInfo.size(); i++)
    {
        nArea = vstRectsInfo[i][2] * vstRectsInfo[i][3];
        if(nArea > nMaxArea)
        {
            nMaxArea = nArea;
            // nMaxAreaIndex = i;
        }
    }

    /* 灵敏度判断：将用户配置的[1,100]转换为[0.3,0.8]进行比较 */
    float fSensitivityThreshold = 0.8f - (m_stSceneChangeDetCfg.nSensitivity - 1) * (0.8f - 0.3f) / (100 - 1);
    float nRegionRatio = (float)nMaxArea / (m_nWidth * m_nHeight);
    /*  检查是否满足报警触发条件 */
    if(nRegionRatio > fSensitivityThreshold)
    {
        /* 把灵敏度映射为[2-10]秒的时间，灵敏度越低触发需要的时间越长 */
        if(time(NULL) - m_duration > ((1.0f - (m_stSceneChangeDetCfg.nSensitivity - 1) * (1.0f - 0.2f) / (100 - 1)) * 10))
        {
            bIsAlarm = true;
            m_duration = time(NULL);
        }
    }
    else 
    {
        m_duration = time(NULL);
    }

    /* 判断是否报警 */
#ifdef ENABLE_TVSDK_SRC
    /* 构建事件触发上下文 */
    EventTriggerContext_S stContext;
    stContext.enEventType = Event::Type_E::SCENE_CHANGE;
    stContext.nChnId = m_nChannelId;
    stContext.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch())
                               .count();

    if (bIsAlarm && !m_stInDataMat.empty())
    {
        auto pPayload = std::make_shared<EventTvSdkPayload_S>();
        pPayload->enType = get_tvsdk_payload_type(stContext.enEventType);
        if (encode_mat_to_tvsdk_image(m_stInDataMat, pPayload->stPanoramaImage))
        {
            stContext.pTvSdkPayload = pPayload;
        }
    }

    m_sceneChangeAlarmStateMachine.handleAlarmState(bIsAlarm, stContext);
#else
    m_sceneChangeAlarmStateMachine.handleAlarmState(bIsAlarm, Event::Type_E::SCENE_CHANGE);
#endif

    return bIsAlarm;
}
