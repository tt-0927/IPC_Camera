/**
 * @FilePath     : hvf_detect.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-29 20:10:37
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-26 15:00:00
 * @Description  : 脸人车侦测
 */

#include "hvf_detect.hpp"

#include <pthread.h>
#include <unistd.h>
#if CAP_AI_PEOPLE_STATISTICS && !CAP_AI_EXHIBITION_PEOPLE_FLOW
#include "action_code.h"
#endif

/* 数据队列最大缓存数量 */
#define QUEUE_MAX (2)

CHVFDetect::CHVFDetect() : m_dateQueue(QUEUE_MAX)
{
    m_bRunning.store(true);
#if CAP_AI_PEOPLE_STATISTICS && !CAP_AI_EXHIBITION_PEOPLE_FLOW
#if CAP_AI_PEOPLE_FLOW_PIPELINE
    /* 设置算法分辨率，Legacy 链路按旧语义消费配置坐标 */
    m_peopleFlowMigrationController.set_algo_resolution(m_nWidth, m_nHeight);
#endif
#endif
#if CAP_UNIFIED_EVENT_PIPELINE
    /* 事件迁移控制器设置算法分辨率 */
    m_eventMigrationController.set_algo_resolution(m_nWidth, m_nHeight);
#endif
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
#if !CAP_UNIFIED_EVENT_PIPELINE
    m_faceProcessor.setEnabled(stAlgoConfig.nEnFaceDetect);
    m_loiteringProcessor.setEnabled(stAlgoConfig.nEnLoiteringDetect);
    m_parkingProcessor.setEnabled(stAlgoConfig.nEnParkingDetect);
    m_boundaryProcessor.setEnabled(stAlgoConfig.nEnLineCrossing);
    m_intrusionProcessor.setEnabled(stAlgoConfig.nEnIntrusion);
    m_enterExitProcessor.setEntranceEnabled(stAlgoConfig.nEnEnterRegion);
    m_enterExitProcessor.setExitEnabled(stAlgoConfig.nEnLeaveRegion);
#endif
#if CAP_AI_PEOPLE_STATISTICS && !CAP_AI_EXHIBITION_PEOPLE_FLOW
    /* 人流统计配置缓存，算法总开关与完整配置一次性交给迁移控制器
     * （展馆自研模型启用时，HVF 人流统计编译关闭，见 CAP_AI_EXHIBITION_PEOPLE_FLOW） */
    Alarm::PeopleFlowStatistics_S stPeopleFlowInfo;
    CEventConfigure::instance()->get_configure(stPeopleFlowInfo);
#if CAP_AI_PEOPLE_FLOW_PIPELINE
    m_peopleFlowMigrationController.apply_config(stPeopleFlowInfo, stAlgoConfig.nEnPeopleFlowStatistics != 0);
#else
    m_peopleFlowProcessor.setEnabled(stAlgoConfig.nEnPeopleFlowStatistics != 0);
    if (m_peopleFlowProcessor.isEnabled())
    {
        setAlgoParamCfg(stPeopleFlowInfo);
    }
#endif
#endif
#if CAP_AI_PEOPLE_DENSITY_V2 && !CAP_AI_PEOPLE_DENSITY_PIPELINE
    /* 迁移宏开启时人员密度由展馆模型 head 链路承接，HVF 不再消费该开关 */
    m_peopleDensityProcessor.setEnabled(stAlgoConfig.nEnPeopleDensityDetection);
#endif

#if !CAP_UNIFIED_EVENT_PIPELINE
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
#endif

#if CAP_UNIFIED_EVENT_PIPELINE
    /* 区域事件族（侵入/进入/离开）统一交迁移控制器原子配置 */
    {
        Alarm::FieldDetection_S stIntrusionInfo;
        CEventConfigure::instance()->get_configure(stIntrusionInfo);
        m_eventMigrationController.apply_intrusion_config(stIntrusionInfo, stAlgoConfig.nEnIntrusion != 0);
    }
    {
        Alarm::EntranceDetection_S stEntranceInfo;
        CEventConfigure::instance()->get_configure(stEntranceInfo);
        m_eventMigrationController.apply_entrance_config(stEntranceInfo, stAlgoConfig.nEnEnterRegion != 0);
    }
    {
        Alarm::ExitingDetection_S stExitInfo;
        CEventConfigure::instance()->get_configure(stExitInfo);
        m_eventMigrationController.apply_exit_config(stExitInfo, stAlgoConfig.nEnLeaveRegion != 0);
    }
    {
        /* 越界事件统一交迁移控制器原子配置 */
        Alarm::BoundaryDetection_S stBoundaryInfo;
        CEventConfigure::instance()->get_configure(stBoundaryInfo);
        m_eventMigrationController.apply_boundary_config(stBoundaryInfo, stAlgoConfig.nEnLineCrossing != 0);
    }
    {
        /* 徘徊侦测统一交迁移控制器原子配置 */
        Alarm::LoiteringDetection_S stLoiteringInfo;
        CEventConfigure::instance()->get_configure(stLoiteringInfo);
        m_eventMigrationController.apply_loitering_config(stLoiteringInfo, stAlgoConfig.nEnLoiteringDetect != 0);
    }
    {
        /* 停车侦测统一交迁移控制器原子配置 */
        Alarm::ParkingDetection_S stParkingInfo;
        CEventConfigure::instance()->get_configure(stParkingInfo);
        m_eventMigrationController.apply_parking_config(stParkingInfo, stAlgoConfig.nEnParkingDetect != 0);
    }
    {
        /* 人脸侦测统一交迁移控制器原子配置 */
        Alarm::FaceDetection_S stFaceInfo;
        CEventConfigure::instance()->get_configure(stFaceInfo);
        m_eventMigrationController.apply_face_config(stFaceInfo, stAlgoConfig.nEnFaceDetect != 0);
    }
#else
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
#endif

#if CAP_AI_PEOPLE_STATISTICS && !CAP_AI_EXHIBITION_PEOPLE_FLOW
#if !CAP_AI_PEOPLE_FLOW_PIPELINE
    if (m_peopleFlowProcessor.isEnabled())
    {
        setAlgoParamCfg(stPeopleFlowInfo);
    }
#endif
#endif

#if CAP_AI_PEOPLE_DENSITY_V2 && !CAP_AI_PEOPLE_DENSITY_PIPELINE
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
#if CAP_UNIFIED_EVENT_PIPELINE
    m_eventMigrationController.update_face_parameters(stAlgoCfg);
#else
    m_faceProcessor.setAlgoParamCfg(stAlgoCfg, m_nWidth, m_nHeight);
#endif
}

void CHVFDetect::setAlgoParamCfg(const Alarm::LoiteringDetection_S &stAlgoCfg)
{
#if CAP_UNIFIED_EVENT_PIPELINE
    m_eventMigrationController.update_loitering_parameters(stAlgoCfg);
#else
    m_loiteringProcessor.setAlgoParamCfg(stAlgoCfg, m_nWidth, m_nHeight);
#endif
}

void CHVFDetect::setAlgoParamCfg(const Alarm::ParkingDetection_S &stAlgoCfg)
{
#if CAP_UNIFIED_EVENT_PIPELINE
    m_eventMigrationController.update_parking_parameters(stAlgoCfg);
#else
    m_parkingProcessor.setAlgoParamCfg(stAlgoCfg, m_nWidth, m_nHeight);
#endif
}

void CHVFDetect::setAlgoParamCfg(const Alarm::BoundaryDetection_S &stAlgoCfg)
{
#if CAP_UNIFIED_EVENT_PIPELINE
    m_eventMigrationController.update_boundary_parameters(stAlgoCfg);
#else
    m_boundaryProcessor.setAlgoParamCfg(stAlgoCfg, m_nWidth, m_nHeight);
#endif
}

void CHVFDetect::setAlgoParamCfg(const Alarm::FieldDetection_S &stAlgoCfg)
{
#if CAP_UNIFIED_EVENT_PIPELINE
    m_eventMigrationController.update_intrusion_parameters(stAlgoCfg);
#else
    m_intrusionProcessor.setAlgoParamCfg(stAlgoCfg, m_nWidth, m_nHeight);
#endif
}

void CHVFDetect::setAlgoParamCfg(const Alarm::EntranceDetection_S &stAlgoCfg)
{
#if CAP_UNIFIED_EVENT_PIPELINE
    m_eventMigrationController.update_entrance_parameters(stAlgoCfg);
#else
    m_enterExitProcessor.setEntranceAlgoParamCfg(stAlgoCfg, m_nWidth, m_nHeight);
#endif
}

void CHVFDetect::setAlgoParamCfg(const Alarm::ExitingDetection_S &stAlgoCfg)
{
#if CAP_UNIFIED_EVENT_PIPELINE
    m_eventMigrationController.update_exit_parameters(stAlgoCfg);
#else
    m_enterExitProcessor.setExitAlgoParamCfg(stAlgoCfg, m_nWidth, m_nHeight);
#endif
}

#if CAP_AI_PEOPLE_STATISTICS && !CAP_AI_EXHIBITION_PEOPLE_FLOW
void CHVFDetect::setAlgoParamCfg(const Alarm::PeopleFlowStatistics_S &stAlgoCfg)
{
#if CAP_AI_PEOPLE_FLOW_PIPELINE
    m_peopleFlowMigrationController.update_parameters(stAlgoCfg);
#else
    m_peopleFlowProcessor.setAlgoParamCfg(stAlgoCfg, m_nWidth, m_nHeight);
#endif
}
#endif

#if CAP_AI_PEOPLE_DENSITY_V2 && !CAP_AI_PEOPLE_DENSITY_PIPELINE
void CHVFDetect::setAlgoParamCfg(const Alarm::PeopleDensityDetection_S &stAlgoCfg)
{
    m_peopleDensityProcessor.setAlgoParamCfg(stAlgoCfg, m_nWidth, m_nHeight);
}
#endif

#if (CAP_AI_PEOPLE_STATISTICS && !CAP_AI_EXHIBITION_PEOPLE_FLOW) || (CAP_AI_PEOPLE_DENSITY_V2 && !CAP_AI_PEOPLE_DENSITY_PIPELINE)
void CHVFDetect::setEventStatisticsReporter(const std::shared_ptr<EventStatistics_NS::IEventStatisticsReporter> &pReporter)
{
#if CAP_AI_PEOPLE_STATISTICS && !CAP_AI_EXHIBITION_PEOPLE_FLOW
    /* 设置人流统计上报器 */
#if CAP_AI_PEOPLE_FLOW_PIPELINE
    m_peopleFlowMigrationController.set_reporter(pReporter);
#else
    m_peopleFlowProcessor.setReporter(pReporter);
#endif
#endif
#if CAP_AI_PEOPLE_DENSITY_V2 && !CAP_AI_PEOPLE_DENSITY_PIPELINE
    /* 设置人员密度 V2 上报器 */
    m_peopleDensityProcessor.setReporter(pReporter);
#endif
}
#endif

#if CAP_AI_PEOPLE_STATISTICS && !CAP_AI_EXHIBITION_PEOPLE_FLOW
int CHVFDetect::handleRuntimeCommand(const RuntimeCommand_S &stCommand)
{
    if (stCommand.nCode != AC_CLEAR_PEOPLE_FLOW_STATISTICS_RESULT)
    {
        return ERR;
    }

#if CAP_AI_PEOPLE_FLOW_PIPELINE
    m_peopleFlowMigrationController.clear_statistics();
#else
    m_peopleFlowProcessor.clearStatisticsResult();
#endif
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

    /* 读取模型输入尺寸，作为检测帧缩放目标；失败时新链路保持不可用且不缩放送帧 */
    ot_aidetect_model_info stModelInfo;
    memset(&stModelInfo, 0, sizeof(stModelInfo));
    if (TD_SUCCESS == ss_mpi_aidetect_get_model_info(AI_DETECT_CHN_HVF, &stModelInfo))
    {
        m_stModelFrameSize.nWidth = stModelInfo.size.width;
        m_stModelFrameSize.nHeight = stModelInfo.size.height;
        dlog_info("HVF 模型输入分辨率[%ux%u]", m_stModelFrameSize.nWidth, m_stModelFrameSize.nHeight);
        if (m_stModelFrameSize.nWidth != static_cast<uint32_t>(m_nWidth) || m_stModelFrameSize.nHeight != static_cast<uint32_t>(m_nHeight))
        {
            dlog_warn("HVF 模型输入[%ux%u]与算法分辨率[%dx%d]不一致，请核对结果坐标基准",
                      m_stModelFrameSize.nWidth,
                      m_stModelFrameSize.nHeight,
                      m_nWidth,
                      m_nHeight);
        }
    }
    else
    {
        dlog_warn("读取 HVF 模型信息失败，检测帧不缩放，新链路保持不可用");
        m_stModelFrameSize = AiPipeline_NS::FrameSize_S();
    }

    /* 创建缩放目标帧，仅创建一次；失败时返回 false 等待重新初始化 */
    if (m_stModelFrameSize.nWidth > 0U && m_stModelFrameSize.nHeight > 0U && 0 == m_stDstFrameInfo.video_frame.width)
    {
        memset_s(&m_stDstFrameInfo, sizeof(ot_video_frame_info), 0, sizeof(ot_video_frame_info));
        if (TD_SUCCESS != mppVgs_create_video_frame_info(m_stModelFrameSize.nWidth,
                                                         m_stModelFrameSize.nHeight,
                                                         OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                                         &m_stDstFrameInfo))
        {
            dlog_error("创建 HVF 缩放目标帧失败 [%ux%u]", m_stModelFrameSize.nWidth, m_stModelFrameSize.nHeight);
            return false;
        }
    }

#if CAP_AI_PEOPLE_STATISTICS && !CAP_AI_EXHIBITION_PEOPLE_FLOW
#if CAP_AI_PEOPLE_FLOW_PIPELINE
    /* 模型输入尺寸同步给新链路 Converter，初始化坐标合同 */
    m_peopleFlowMigrationController.set_model_input_size(m_stModelFrameSize);
#endif
#endif
#if CAP_UNIFIED_EVENT_PIPELINE
    /* 模型输入尺寸同步给事件迁移控制器 */
    m_eventMigrationController.set_model_input_size(m_stModelFrameSize);
#endif
    return true;
}

bool CHVFDetect::unInit()
{
    if (m_pHVFDetHandle)
    {
        streamAiDetect_uninit(m_pHVFDetHandle);
        m_pHVFDetHandle = nullptr;
    }
    if (0 != m_stDstFrameInfo.video_frame.width)
    {
        mppVgs_destroy_video_frame_info(&m_stDstFrameInfo);
        memset_s(&m_stDstFrameInfo, sizeof(ot_video_frame_info), 0, sizeof(ot_video_frame_info));
    }
    m_stModelFrameSize = AiPipeline_NS::FrameSize_S();
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

        /* 送入模型的帧：源帧分辨率与模型输入不一致时先缩放，事件处理器仍使用原始帧编码图片 */
        ot_video_frame_info *pSendFrameInfo = pFrameInfo;
        if (m_stModelFrameSize.nWidth > 0U && m_stModelFrameSize.nHeight > 0U &&
            (static_cast<uint32_t>(pFrameInfo->video_frame.width) != m_stModelFrameSize.nWidth ||
             static_cast<uint32_t>(pFrameInfo->video_frame.height) != m_stModelFrameSize.nHeight))
        {
            if (TD_SUCCESS != mppVgs_scale(pFrameInfo, &m_stDstFrameInfo))
            {
                dlog_error("HVF 检测帧缩放失败 [%ux%u] -> [%ux%u]",
                           pFrameInfo->video_frame.width,
                           pFrameInfo->video_frame.height,
                           m_stModelFrameSize.nWidth,
                           m_stModelFrameSize.nHeight);
                continue;
            }
            pSendFrameInfo = &m_stDstFrameInfo;
        }

        if (m_pHVFDetHandle->svpAiDetect_sendFrame(m_pHVFDetHandle, &pSendFrameInfo->video_frame) != TD_SUCCESS)
        {
            continue;
        }

        /* 当前帧汇总角框输出数组 */
        std::vector<Common::RectInfo_S> vstRectInfo;

#if CAP_AI_PEOPLE_STATISTICS && !CAP_AI_EXHIBITION_PEOPLE_FLOW
#if CAP_AI_PEOPLE_FLOW_PIPELINE
        if (m_peopleFlowMigrationController.is_enabled())
        {
            /* 当前帧源帧尺寸，作为标准检测结果元数据 */
            AiPipeline_NS::FrameSize_S stSourceFrameSize{ static_cast<uint32_t>(pFrameInfo->video_frame.width),
                                                          static_cast<uint32_t>(pFrameInfo->video_frame.height) };
            m_peopleFlowMigrationController.process(m_pHVFDetHandle->stResult,
                                                    stMediaData.stMediaParam.nChannel,
                                                    stMediaData.pVideoFrameInfo,
                                                    stSourceFrameSize,
                                                    vstRectInfo);
        }
#else
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
#endif

#if CAP_AI_PEOPLE_DENSITY_V2 && !CAP_AI_PEOPLE_DENSITY_PIPELINE
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

#if !CAP_UNIFIED_EVENT_PIPELINE
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
#endif

#if CAP_UNIFIED_EVENT_PIPELINE
        if (m_eventMigrationController.is_enabled())
        {
            AiPipeline_NS::FrameSize_S stSourceFrameSize{ static_cast<uint32_t>(pFrameInfo->video_frame.width),
                                                          static_cast<uint32_t>(pFrameInfo->video_frame.height) };
            m_eventMigrationController.process(m_pHVFDetHandle->stResult,
                                               stMediaData.stMediaParam.nChannel,
                                               stMediaData.pVideoFrameInfo,
                                               stSourceFrameSize,
                                               vstRectInfo);
        }
#else
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
#endif

#if !CAP_UNIFIED_EVENT_PIPELINE
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
#endif

#if !CAP_UNIFIED_EVENT_PIPELINE
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
#endif

        if (false
#if !CAP_UNIFIED_EVENT_PIPELINE
            || m_loiteringProcessor.isEnabled() || m_parkingProcessor.isEnabled() ||
            m_boundaryProcessor.isEnabled() || m_intrusionProcessor.isEnabled() || m_enterExitProcessor.isEntranceEnabled() || m_enterExitProcessor.isExitEnabled()
#else
            || m_eventMigrationController.is_enabled()
#endif
#if CAP_AI_PEOPLE_STATISTICS && !CAP_AI_EXHIBITION_PEOPLE_FLOW
#if CAP_AI_PEOPLE_FLOW_PIPELINE
            || m_peopleFlowMigrationController.is_enabled()
#else
            || m_peopleFlowProcessor.isEnabled()
#endif
#endif
#if CAP_AI_PEOPLE_DENSITY_V2 && !CAP_AI_PEOPLE_DENSITY_PIPELINE
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
    return false
#if CAP_UNIFIED_EVENT_PIPELINE
           || m_eventMigrationController.is_enabled()
#else
           || m_faceProcessor.isEnabled() || m_loiteringProcessor.isEnabled() || m_parkingProcessor.isEnabled() ||
           m_boundaryProcessor.isEnabled() || m_intrusionProcessor.isEnabled() || m_enterExitProcessor.isEntranceEnabled() || m_enterExitProcessor.isExitEnabled()
#endif
#if CAP_AI_PEOPLE_STATISTICS && !CAP_AI_EXHIBITION_PEOPLE_FLOW
#if CAP_AI_PEOPLE_FLOW_PIPELINE
           || m_peopleFlowMigrationController.is_enabled()
#else
           || m_peopleFlowProcessor.isEnabled()
#endif
#endif
#if CAP_AI_PEOPLE_DENSITY_V2 && !CAP_AI_PEOPLE_DENSITY_PIPELINE
           || m_peopleDensityProcessor.isEnabled()
#endif
        ;
}
