/**
 * @FilePath     : people_head_detect.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-22 18:44:55
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-23 09:39:03
 * @Description  : 人头检测主控模块实现
 */

#include "people_head_detect.hpp"

#include <pthread.h>
#include <string>
#include <unistd.h>

/* 数据队列最大缓存数量 */
#define QUEUE_MAX (2)

CPeopleHeadDetect::CPeopleHeadDetect()
    : m_dateQueue(QUEUE_MAX)
{
    m_bRunning.store(true);
    m_thread = std::thread(&CPeopleHeadDetect::run, this);
}

CPeopleHeadDetect::~CPeopleHeadDetect()
{
    /* 通知线程停止并唤醒阻塞队列，避免析构时线程无法退出 */
    m_bRunning.store(false);
    m_dateQueue.shutdown();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
    m_dateQueue.clear();
    unInit();
}

void CPeopleHeadDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!hasEnabledAlgorithm())
    {
        return;
    }

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("人头检测-数据队列满了 [%d]", m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

void CPeopleHeadDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_crowdGatheringProcessor.setEnabled(stAlgoConfig.nEnCrowdGathering);
#if CAP_AI_PEOPLE_DENSITY_LEGACY
    m_densityProcessor.setEnabled(stAlgoConfig.nEnPeopleDensityDetection);
#endif

    if (m_crowdGatheringProcessor.isEnabled())
    {
        Alarm::CrowdGathering_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

#if CAP_AI_PEOPLE_DENSITY_LEGACY
    if (m_densityProcessor.isEnabled())
    {
        Alarm::PeopleDensityDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
#endif
}

void CPeopleHeadDetect::setAlgoParamCfg(const Alarm::CrowdGathering_S &stAlgoCfg)
{
    m_crowdGatheringProcessor.setAlgoParamCfg(stAlgoCfg, m_nWidth, m_nHeight);
}

#if CAP_AI_PEOPLE_DENSITY_LEGACY
void CPeopleHeadDetect::setAlgoParamCfg(const Alarm::PeopleDensityDetection_S &stAlgoCfg)
{
    m_densityProcessor.setAlgoParamCfg(stAlgoCfg, m_nWidth, m_nHeight);
}

void CPeopleHeadDetect::setEventStatisticsReporter(
    const std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> &pReporter)
{
    m_densityProcessor.setReporter(pReporter);
}
#endif

bool CPeopleHeadDetect::init()
{
    if (m_pHeadDetHandle)
    {
        return true;
    }

    const std::string strModelPath = AI_HEAD_DETECTION_CONFIG_FILE;
    m_pHeadDetHandle = new Inference_NS::CYoloUltralytics(strModelPath);
    if (!m_pHeadDetHandle)
    {
        dlog_error("人头检测初始化失败-创建模型对象失败");
        return false;
    }

    if (!m_pHeadDetHandle->init())
    {
        delete m_pHeadDetHandle;
        m_pHeadDetHandle = nullptr;
        dlog_error("人头检测初始化失败");
        return false;
    }

    dlog_info("人头检测初始化成功, %s", strModelPath.c_str());
    memset_s(&m_stDstFrameInfo, sizeof(ot_video_frame_info), 0, sizeof(ot_video_frame_info));
    if (TD_SUCCESS != mppVgs_create_video_frame_info(m_nWidth,
                                                     m_nHeight,
                                                     OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                                     &m_stDstFrameInfo))
    {
        delete m_pHeadDetHandle;
        m_pHeadDetHandle = nullptr;
        dlog_error("人头检测初始化失败-创建目标视频帧失败");
        return false;
    }

    return true;
}

bool CPeopleHeadDetect::unInit()
{
    mppVgs_destroy_video_frame_info(&m_stDstFrameInfo);
    if (m_pHeadDetHandle)
    {
        delete m_pHeadDetHandle;
        m_pHeadDetHandle = nullptr;
    }

    return true;
}

void CPeopleHeadDetect::run()
{
    pthread_setname_np(pthread_self(), "PeopleHead");

    /* 当前线程从队列取出的媒体数据 */
    MediaData_S stMediaData;
    while (m_bRunning.load())
    {
        if (!m_pHeadDetHandle)
        {
            if (!hasEnabledAlgorithm())
            {
                sleep(1);
                continue;
            }

            if (!init())
            {
                dlog_error("等待人头检测初始化");
                std::this_thread::sleep_for(std::chrono::seconds(1));
                if (!m_bRunning.load())
                {
                    break;
                }
                continue;
            }
        }

        if (!m_dateQueue.pop(stMediaData, TIMEOUT_1000_MS) || stMediaData.pVideoFrameInfo == nullptr)
        {
            continue;
        }

        /* 当前原始视频帧指针，队列中使用 shared_ptr 托管生命周期 */
        ot_video_frame_info *pSrcFrameInfo = stMediaData.pVideoFrameInfo.get();
        if (!pSrcFrameInfo)
        {
            dlog_error("原始数据帧为空");
            continue;
        }

        /* 当前帧是否需要缩放到人头模型输入分辨率 */
        bool bIsScale = false;
        if (m_nWidth != stMediaData.stMediaParam.nVideoWidth || m_nHeight != stMediaData.stMediaParam.nVideoHeight)
        {
            bIsScale = true;
        }

        /* 实际送入模型的人头检测视频帧，默认直接使用原始帧 */
        ot_video_frame_info *pFrameInfo = pSrcFrameInfo;
        if (bIsScale)
        {
            if (TD_SUCCESS != mppVgs_scale(pSrcFrameInfo, &m_stDstFrameInfo))
            {
                continue;
            }
            pFrameInfo = &m_stDstFrameInfo;
        }

        /* 人头模型输入数据，指向待推理视频帧的 YUV 数据地址 */
        Inference_NS::InputData_S stInputData;
        stInputData.pData = (float *)pFrameInfo->video_frame.virt_addr[0];
        stInputData.nDataSize = static_cast<int>(m_nWidth * m_nHeight * 1.5) * sizeof(float);

        /* 当前帧人头检测模型输出框集合 */
        std::vector<Inference_NS::BoxData_S> vBoxDatas;
        m_pHeadDetHandle->inference(stInputData, vBoxDatas);

        /* 当前帧需要发送给 OSD 的目标框集合 */
        std::vector<Common::RectInfo_S> vstRectInfo;
        /* 当前帧处理时间戳，供聚集和密度处理器共用 */
        const long long llNowMs = static_cast<long long>(get_time_ms());

        if (m_crowdGatheringProcessor.isEnabled())
        {
#if CAP_EXHIBITION_OSD_PANEL
            /* 人员聚集展会面板结果 */
            OsdPanel::PanelFrame_S stCrowdPanelFrame;
            /* 人员聚集处理上下文，携带模型输出、OSD结果和面板输出 */
            PeopleHeadDetectInternal::SPeopleHeadProcessContext stContext{ vBoxDatas,
                                                                           vstRectInfo,
                                                                           m_nWidth,
                                                                            m_nHeight,
                                                                            llNowMs,
                                                                            stMediaData.stMediaParam.nChannel,
                                                                            &stCrowdPanelFrame,
                                                                            pFrameInfo };
#else
            /* 人员聚集处理上下文，携带模型输出和 OSD 结果 */
            PeopleHeadDetectInternal::SPeopleHeadProcessContext stContext{ vBoxDatas,
                                                                           vstRectInfo,
                                                                           m_nWidth,
                                                                           m_nHeight,
                                                                           llNowMs,
                                                                           stMediaData.stMediaParam.nChannel,
                                                                           pFrameInfo };
#endif
            m_crowdGatheringProcessor.process(stContext);
#if CAP_EXHIBITION_OSD_PANEL
            send_panelResult_to_osd(stCrowdPanelFrame);
#endif
        }

#if CAP_AI_PEOPLE_DENSITY_LEGACY
        if (m_densityProcessor.isEnabled())
        {
            /* 人员密度处理上下文，复用同一帧人头模型输出 */
            PeopleHeadDetectInternal::SPeopleHeadProcessContext stContext{ vBoxDatas,
                                                                           vstRectInfo,
                                                                           m_nWidth,
                                                                           m_nHeight,
                                                                           llNowMs,
                                                                           stMediaData.stMediaParam.nChannel
#if CAP_EXHIBITION_OSD_PANEL
                                                                            ,
                                                                            nullptr,
                                                                            pFrameInfo
#else
                                                                            ,
                                                                            pFrameInfo
#endif
            };
            m_densityProcessor.process(stContext);
        }
#endif

        if (!vstRectInfo.empty())
        {
            send_detectionResult_to_osd(m_nWidth, m_nHeight, vstRectInfo);
        }
    }
}

bool CPeopleHeadDetect::hasEnabledAlgorithm() const
{
    return m_crowdGatheringProcessor.isEnabled()
#if CAP_AI_PEOPLE_DENSITY_LEGACY
           || m_densityProcessor.isEnabled()
#endif
    ;
}
