/**
 * @FilePath     : hvf_detect.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-29 20:10:37
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-28 10:43:37
 * @Description  : 脸人车侦测
 */

#include "hvf_detect.hpp"

#include <pthread.h>
#include <unistd.h>
#if CAP_AI_PEOPLE_STATISTICS
#include "action_code.h"
#endif

/* 数据队列最大缓存数量 */
#define QUEUE_MAX (2)

CHVFDetect::CHVFDetect() : m_dateQueue(QUEUE_MAX)
{
    m_bRunning.store(true);
    m_thread = std::thread(&CHVFDetect::run, this);
}

CHVFDetect::~CHVFDetect()
{
    m_bRunning.store(false);
    m_dateQueue.shutdown();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
    m_dateQueue.clear();
    unInit();
}

void CHVFDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!hasEnabledAlgorithm())
    {
        return;
    }

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("脸人车侦测-数据队列满了 [%d]", m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

void CHVFDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_faceProcessor.setEnabled(stAlgoConfig.nEnFaceDetect);
    m_loiteringProcessor.setEnabled(stAlgoConfig.nEnLoiteringDetect);
    m_parkingProcessor.setEnabled(stAlgoConfig.nEnParkingDetect);
    m_boundaryProcessor.setEnabled(stAlgoConfig.nEnLineCrossing);
    m_intrusionProcessor.setEnabled(stAlgoConfig.nEnIntrusion);
    m_enterExitProcessor.setEntranceEnabled(stAlgoConfig.nEnEnterRegion);
    m_enterExitProcessor.setExitEnabled(stAlgoConfig.nEnLeaveRegion);
#if CAP_AI_PEOPLE_STATISTICS
    m_peopleFlowProcessor.setEnabled(stAlgoConfig.nEnPeopleFlowStatistics);
#endif
#if CAP_AI_PEOPLE_DENSITY_V2
    m_peopleDensityProcessor.setEnabled(stAlgoConfig.nEnPeopleDensityDetection);
#endif

    if (m_faceProcessor.isEnabled())
    {
        /* 人脸侦测配置缓存 */
        Alarm::FaceDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_loiteringProcessor.isEnabled())
    {
        /* 徘徊侦测配置缓存 */
        Alarm::LoiteringDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_parkingProcessor.isEnabled())
    {
        /* 停车侦测配置缓存 */
        Alarm::ParkingDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_boundaryProcessor.isEnabled())
    {
        /* 越界侦测配置缓存 */
        Alarm::BoundaryDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_intrusionProcessor.isEnabled())
    {
        /* 区域入侵配置缓存 */
        Alarm::FieldDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_enterExitProcessor.isEntranceEnabled())
    {
        /* 进入区域配置缓存 */
        Alarm::EntranceDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

    if (m_enterExitProcessor.isExitEnabled())
    {
        /* 离开区域配置缓存 */
        Alarm::ExitingDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }

#if CAP_AI_PEOPLE_STATISTICS
    if (m_peopleFlowProcessor.isEnabled())
    {
        /* 人流统计配置缓存 */
        Alarm::PeopleFlowStatistics_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
#endif

#if CAP_AI_PEOPLE_DENSITY_V2
    if (m_peopleDensityProcessor.isEnabled())
    {
        /* 人员密度配置缓存 */
        Alarm::PeopleDensityDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
#endif
}

void CHVFDetect::setAlgoParamCfg(const Alarm::FaceDetection_S &stAlgoCfg)
{
    m_faceProcessor.setAlgoParamCfg(stAlgoCfg, m_nWidth, m_nHeight);
}

void CHVFDetect::setAlgoParamCfg(const Alarm::LoiteringDetection_S &stAlgoCfg)
{
    m_loiteringProcessor.setAlgoParamCfg(stAlgoCfg, m_nWidth, m_nHeight);
}

void CHVFDetect::setAlgoParamCfg(const Alarm::ParkingDetection_S &stAlgoCfg)
{
    m_parkingProcessor.setAlgoParamCfg(stAlgoCfg, m_nWidth, m_nHeight);
}

void CHVFDetect::setAlgoParamCfg(const Alarm::BoundaryDetection_S &stAlgoCfg)
{
    m_boundaryProcessor.setAlgoParamCfg(stAlgoCfg, m_nWidth, m_nHeight);
}

void CHVFDetect::setAlgoParamCfg(const Alarm::FieldDetection_S &stAlgoCfg)
{
    m_intrusionProcessor.setAlgoParamCfg(stAlgoCfg, m_nWidth, m_nHeight);
}

void CHVFDetect::setAlgoParamCfg(const Alarm::EntranceDetection_S &stAlgoCfg)
{
    m_enterExitProcessor.setEntranceAlgoParamCfg(stAlgoCfg, m_nWidth, m_nHeight);
}

void CHVFDetect::setAlgoParamCfg(const Alarm::ExitingDetection_S &stAlgoCfg)
{
    m_enterExitProcessor.setExitAlgoParamCfg(stAlgoCfg, m_nWidth, m_nHeight);
}

#if CAP_AI_PEOPLE_STATISTICS
void CHVFDetect::setAlgoParamCfg(const Alarm::PeopleFlowStatistics_S &stAlgoCfg)
{
    m_peopleFlowProcessor.setAlgoParamCfg(stAlgoCfg, m_nWidth, m_nHeight);
}
#endif

#if CAP_AI_PEOPLE_DENSITY_V2
void CHVFDetect::setAlgoParamCfg(const Alarm::PeopleDensityDetection_S &stAlgoCfg)
{
    m_peopleDensityProcessor.setAlgoParamCfg(stAlgoCfg, m_nWidth, m_nHeight);
}
#endif

#if CAP_AI_PEOPLE_STATISTICS || CAP_AI_PEOPLE_DENSITY_V2
void CHVFDetect::setEventStatisticsReporter(const std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> &pReporter)
{
#if CAP_AI_PEOPLE_STATISTICS
    /* 设置人流统计上报器 */
    m_peopleFlowProcessor.setReporter(pReporter);
#endif
#if CAP_AI_PEOPLE_DENSITY_V2
    /* 设置人员密度 V2 上报器 */
    m_peopleDensityProcessor.setReporter(pReporter);
#endif
}
#endif

#if CAP_AI_PEOPLE_STATISTICS
int CHVFDetect::handleRuntimeCommand(const RuntimeCommand_S &stCommand)
{
    if (stCommand.nCode != AC_CLEAR_PEOPLE_FLOW_STATISTICS_RESULT)
    {
        return ERR;
    }

    m_peopleFlowProcessor.clearStatisticsResult();
    dlog_info("人流统计运行态结果已清零");
    return OK;
}
#endif

bool CHVFDetect::init()
{
    if (!m_pHVFDetHandle)
    {
        m_pHVFDetHandle = streamAiDetect_init(AI_DETECT_CHN_HVF, AI_HVF_NORMAL_MODEL_PATH);
        if (!m_pHVFDetHandle)
        {
            dlog_error("脸人车侦测初始化失败");
            return false;
        }
        dlog_info("脸人车侦测初始化成功");
    }
    // if (0 == m_stDstFrameInfo.video_frame.width)
    // {
    //     memset_s(&m_stDstFrameInfo, sizeof(ot_video_frame_info), 0, sizeof(ot_video_frame_info));

    //     if (TD_SUCCESS !=
    //         mppVgs_create_video_frame_info(m_nWidth, m_nHeight, OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420, &m_stDstFrameInfo))
    //     {
    //         dlog_error("创建目标视频帧失败");

    //         return false;
    //     }
    // }
    return true;
}

bool CHVFDetect::unInit()
{
    if (m_pHVFDetHandle)
    {
        streamAiDetect_uninit(m_pHVFDetHandle);
        m_pHVFDetHandle = nullptr;
    }
    // mppVgs_destroy_video_frame_info(&m_stDstFrameInfo);
    // memset_s(&m_stDstFrameInfo, sizeof(ot_video_frame_info), 0, sizeof(ot_video_frame_info));
    return true;
}

bool CHVFDetect::reboot()
{
    if (!unInit())
    {
        return false;
    }

    if (!init())
    {
        return false;
    }

    return true;
}

void CHVFDetect::run()
{
    pthread_setname_np(pthread_self(), "HvfDetect");

    /* 当前线程从队列取出的媒体数据 */
    MediaData_S stMediaData;

    while (m_bRunning.load())
    {
        if (!m_pHVFDetHandle)
        {
            if (!hasEnabledAlgorithm())
            {
                sleep(1);
                continue;
            }

            if (!init())
            {
                dlog_error("等待脸人车侦测初始化");
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

        /* 当前帧原始视频帧指针 */
        ot_video_frame_info *pFrameInfo = stMediaData.pVideoFrameInfo.get();
        if (!pFrameInfo)
        {
            dlog_error("原始数据帧为空");
            continue;
        }
        // ot_video_frame_info *pSrcFrameInfo = stMediaData.pVideoFrameInfo.get();
        // if (!pSrcFrameInfo)
        // {
        //     dlog_error("原始数据帧为空");
        //     continue;
        // }
        // bool bIsScale = false;

        // if (m_nWidth != stMediaData.stMediaParam.nVideoWidth || m_nHeight != stMediaData.stMediaParam.nVideoHeight)
        // {
        //     bIsScale = true;
        // }

        // /*
        //  * 算法输入帧
        //  */
        // ot_video_frame_info *pFrameInfo = pSrcFrameInfo;

        // /*
        //  * 缩放到检测分辨率
        //  */
        // if (bIsScale)
        // {
        //     if (TD_SUCCESS != mppVgs_scale(pSrcFrameInfo, &m_stDstFrameInfo))
        //     {
        //         dlog_error("mppVgs_scale失败");

        //         continue;
        //     }

        //     pFrameInfo = &m_stDstFrameInfo;
        // }
        
        if (m_pHVFDetHandle->svpAiDetect_sendFrame(m_pHVFDetHandle, &pFrameInfo->video_frame) != TD_SUCCESS)
        {
            continue;
        }

        /* 当前帧汇总角框输出数组 */
        std::vector<Common::RectInfo_S> vstRectInfo;

#if CAP_AI_PEOPLE_STATISTICS
        if (m_peopleFlowProcessor.isEnabled())
        {
            /* 人流统计处理上下文 */
            HVFDetectInternal::SHVFProcessContext stContext{ m_pHVFDetHandle->stResult,
                                                             vstRectInfo,
                                                             m_nWidth,
                                                             m_nHeight
#if CAP_EXHIBITION_OSD_PANEL
                                                             ,
                                                             nullptr
#endif
            };
            stContext.nChnId = stMediaData.stMediaParam.nChannel;
            stContext.llTimestamp = TimeUtils_NS::get_currentTimestampMs();
            stContext.pFrameInfo = pFrameInfo;
            m_peopleFlowProcessor.process(stContext);
        }
#endif

#if CAP_AI_PEOPLE_DENSITY_V2
        if (m_peopleDensityProcessor.isEnabled())
        {
            /* 人员密度 V2 处理上下文，复用同一帧 HVF 人形模型输出 */
            HVFDetectInternal::SHVFProcessContext stContext{ m_pHVFDetHandle->stResult,
                                                             vstRectInfo,
                                                             m_nWidth,
                                                             m_nHeight
#if CAP_EXHIBITION_OSD_PANEL
                                                             ,
                                                             nullptr
#endif
            };
            stContext.nChnId = stMediaData.stMediaParam.nChannel;
            stContext.llTimestamp = TimeUtils_NS::get_currentTimestampMs();
            stContext.pFrameInfo = pFrameInfo;
            m_peopleDensityProcessor.process(stContext);
        }
#endif

        if (m_boundaryProcessor.isEnabled())
        {
            /* 越界侦测处理上下文 */
            HVFDetectInternal::SHVFProcessContext stContext{ m_pHVFDetHandle->stResult,
                                                             vstRectInfo,
                                                             m_nWidth,
                                                             m_nHeight
#if CAP_EXHIBITION_OSD_PANEL
                                                             ,
                                                             nullptr
#endif
            };
            stContext.nChnId = stMediaData.stMediaParam.nChannel;
            stContext.llTimestamp = TimeUtils_NS::get_currentTimestampMs();
            stContext.pFrameInfo = pFrameInfo;
            m_boundaryProcessor.process(stContext);
        }

        if (m_intrusionProcessor.isEnabled())
        {
#if CAP_EXHIBITION_OSD_PANEL
            /* 区域入侵展会面板结果 */
            OsdPanel::PanelFrame_S stIntrusionPanelFrame;
            /* 区域入侵处理上下文 */
            HVFDetectInternal::SHVFProcessContext stContext{ m_pHVFDetHandle->stResult,
                                                             vstRectInfo,
                                                             m_nWidth,
                                                             m_nHeight,
                                                             &stIntrusionPanelFrame };
#else
            /* 区域入侵处理上下文 */
            HVFDetectInternal::SHVFProcessContext stContext{ m_pHVFDetHandle->stResult, vstRectInfo, m_nWidth, m_nHeight };
#endif
            stContext.nChnId = stMediaData.stMediaParam.nChannel;
            stContext.llTimestamp = TimeUtils_NS::get_currentTimestampMs();
            stContext.pFrameInfo = pFrameInfo;
            m_intrusionProcessor.process(stContext);
#if CAP_EXHIBITION_OSD_PANEL
            send_panelResult_to_osd(stIntrusionPanelFrame);
#endif
        }

        if (m_enterExitProcessor.isEntranceEnabled())
        {
            /* 进入区域处理上下文 */
            HVFDetectInternal::SHVFProcessContext stContext{ m_pHVFDetHandle->stResult,
                                                             vstRectInfo,
                                                             m_nWidth,
                                                             m_nHeight
#if CAP_EXHIBITION_OSD_PANEL
                                                             ,
                                                             nullptr
#endif
            };
            stContext.nChnId = stMediaData.stMediaParam.nChannel;
            stContext.llTimestamp = TimeUtils_NS::get_currentTimestampMs();
            stContext.pFrameInfo = pFrameInfo;
            m_enterExitProcessor.processEntrance(stContext);
        }

        if (m_enterExitProcessor.isExitEnabled())
        {
            /* 离开区域处理上下文 */
            HVFDetectInternal::SHVFProcessContext stContext{ m_pHVFDetHandle->stResult,
                                                             vstRectInfo,
                                                             m_nWidth,
                                                             m_nHeight
#if CAP_EXHIBITION_OSD_PANEL
                                                             ,
                                                             nullptr
#endif
            };
            stContext.nChnId = stMediaData.stMediaParam.nChannel;
            stContext.llTimestamp = TimeUtils_NS::get_currentTimestampMs();
            stContext.pFrameInfo = pFrameInfo;
            m_enterExitProcessor.processExit(stContext);
        }

        if (m_faceProcessor.isEnabled())
        {
            /* 人脸侦测处理上下文 */
            HVFDetectInternal::SHVFProcessContext stContext{ m_pHVFDetHandle->stResult,
                                                             vstRectInfo,
                                                             m_nWidth,
                                                             m_nHeight
#if CAP_EXHIBITION_OSD_PANEL
                                                             ,
                                                             nullptr
#endif
            };
            stContext.nChnId = stMediaData.stMediaParam.nChannel;
            stContext.llTimestamp = TimeUtils_NS::get_currentTimestampMs();
            stContext.pFrameInfo = pFrameInfo;
            m_faceProcessor.process(stContext);
        }

        if (m_loiteringProcessor.isEnabled())
        {
#if CAP_EXHIBITION_OSD_PANEL
            /* 徘徊侦测展会面板结果 */
            OsdPanel::PanelFrame_S stLoiteringPanelFrame;
            /* 徘徊侦测处理上下文 */
            HVFDetectInternal::SHVFProcessContext stContext{ m_pHVFDetHandle->stResult,
                                                             vstRectInfo,
                                                             m_nWidth,
                                                             m_nHeight,
                                                             &stLoiteringPanelFrame };
#else
            /* 徘徊侦测处理上下文 */
            HVFDetectInternal::SHVFProcessContext stContext{ m_pHVFDetHandle->stResult, vstRectInfo, m_nWidth, m_nHeight };
#endif
            stContext.nChnId = stMediaData.stMediaParam.nChannel;
            stContext.llTimestamp = TimeUtils_NS::get_currentTimestampMs();
            stContext.pFrameInfo = pFrameInfo;
            m_loiteringProcessor.process(stContext);
#if CAP_EXHIBITION_OSD_PANEL
            send_panelResult_to_osd(stLoiteringPanelFrame);
#endif
        }

        if (m_parkingProcessor.isEnabled())
        {
            /* 停车侦测处理上下文 */
            HVFDetectInternal::SHVFProcessContext stContext{ m_pHVFDetHandle->stResult,
                                                             vstRectInfo,
                                                             m_nWidth,
                                                             m_nHeight
#if CAP_EXHIBITION_OSD_PANEL
                                                             ,
                                                             nullptr
#endif
            };
            stContext.nChnId = stMediaData.stMediaParam.nChannel;
            stContext.llTimestamp = TimeUtils_NS::get_currentTimestampMs();
            stContext.pFrameInfo = pFrameInfo;
            m_parkingProcessor.process(stContext);
        }

        if (m_boundaryProcessor.isEnabled() || m_intrusionProcessor.isEnabled() || m_loiteringProcessor.isEnabled() ||
            m_parkingProcessor.isEnabled() || m_enterExitProcessor.isEntranceEnabled() || m_enterExitProcessor.isExitEnabled()
#if CAP_AI_PEOPLE_STATISTICS
            || m_peopleFlowProcessor.isEnabled()
#endif
#if CAP_AI_PEOPLE_DENSITY_V2
            || m_peopleDensityProcessor.isEnabled()
#endif
        )
        {
            send_detectionResult_to_osd(m_nWidth, m_nHeight, vstRectInfo);
        }
    }
}

bool CHVFDetect::hasEnabledAlgorithm() const
{
    return m_faceProcessor.isEnabled() || m_loiteringProcessor.isEnabled() || m_parkingProcessor.isEnabled() ||
           m_boundaryProcessor.isEnabled() || m_intrusionProcessor.isEnabled() || m_enterExitProcessor.isEntranceEnabled() ||
           m_enterExitProcessor.isExitEnabled()
#if CAP_AI_PEOPLE_STATISTICS
           || m_peopleFlowProcessor.isEnabled()
#endif
#if CAP_AI_PEOPLE_DENSITY_V2
           || m_peopleDensityProcessor.isEnabled()
#endif
    ;
}
