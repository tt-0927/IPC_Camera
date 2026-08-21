/**
 * @FilePath     : hide_detect.cpp
 * @Author       : cyc
 * @Date         : 2025-07-21 15:46:58
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-15 10:45:54
 * @Description  : 遮挡检测
 */

#include "hide_detect.hpp"
#include "video_frame_jpeg_encoder.hpp"
#include "algo_stream_deal.h"

/* 数据队列 */
#define QUEUE_MAX (2)

CHideDetect::CHideDetect()
    : m_dateQueue(QUEUE_MAX)
{
    /* 默认侦测区域 */
    m_stRect.nWidth = m_nWidth;
    m_stRect.nHeight = m_nHeight;

    /* 启动线程 */
    m_bRunning.store(true);
    m_thread = std::thread(&CHideDetect::run, this);
}

CHideDetect::~CHideDetect()
{
    /* 通知线程停止 */
    m_bRunning.store(false);
    // m_condition.notify_all();
    // note 调用 shutdown() 来唤醒可能阻塞在 pop() 的线程
    m_dateQueue.shutdown();
    if (m_thread.joinable())
    {
        m_thread.join();
    }
    m_dateQueue.clear();
    unInit();
}

void CHideDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_stAlgoHideDetCfg.bEnable)
    {
        return;
    }

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("遮挡检测-数据队列满了 [%d]", m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

void CHideDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stAlgoHideDetCfg.bEnable = stAlgoConfig.nEnOcclusionDetect;
    if(m_stAlgoHideDetCfg.bEnable)
    {
        Alarm::HideAlarm_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
}

void CHideDetect::setAlgoParamCfg(const Alarm::HideAlarm_S &stAlgoCfg)
{
    dlog_debug("ai_app: 设置遮挡侦测参数");
    m_stAlgoHideDetCfg = stAlgoCfg;

    convertResolutionAndEnable(m_stAlgoHideDetCfg);

    /* 用户设定的区域 */
    Common::Rect_S stUser = m_stAlgoHideDetCfg.stRect;

    if (stUser.isEmpty())
    {
        /* 未设置区域，不使能算法 */
        m_stAlgoHideDetCfg.bEnable = false;
    }
    else
    {
        /* 对齐 IVE 要求 */
        stUser.nX = ALIGN_BACK(stUser.nX, 16);
        stUser.nY = ALIGN_BACK(stUser.nY, 4);
        stUser.nWidth = ALIGN_BACK(stUser.nWidth, 16);
        stUser.nHeight = ALIGN_BACK(stUser.nHeight, 4);

        dlog_debug("[遮挡侦测] : m_stRect: [%d,%d][%d,%d]", m_stRect.nX, m_stRect.nY, m_stRect.nWidth, m_stRect.nHeight);
        dlog_debug("[遮挡侦测] :   stUser: [%d,%d][%d,%d]", stUser.nX, stUser.nY, stUser.nWidth, stUser.nHeight);

        /* 宽高变化 => 重初始化 */
        if ((m_stRect.nWidth != stUser.nWidth) || (m_stRect.nHeight != stUser.nHeight))
        {
            m_stRect = stUser;
            m_bNeedReInit.store(true);
        }
    }
}

/**
 * @brief   : 转换区域坐标并判断是否使能算法
 */
 template <typename T> 
 void CHideDetect::convertResolutionAndEnable(T &stConfig)
 {

         /* 是否有任何一个区域初始化成功 */
         bool bIsInit = false;
 
         stConfig.stRect.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight);
        /* 判断是否设置了正确的多边形 */
        if (stConfig.stRect.IsValid())
        {
            bIsInit = true;
            /* 不要提前返回，继续转换其他区域 */
        }
         
 
         /* 没有一个正确的多边形区域，不使能 */
         if (!bIsInit)
         {
             stConfig.bEnable = false;
         }
     
 }

bool CHideDetect::init()
{
    if (!m_pHideDetHandle)
    {      
        HiOdNeedParam_S stNeedParam;
        stNeedParam.nWidth = m_stRect.nWidth;
        stNeedParam.nHeight = m_stRect.nHeight;
        m_pHideDetHandle = svpOd_alloc(stNeedParam);
        if (m_pHideDetHandle)
        {
            if (TD_SUCCESS == m_pHideDetHandle->svpOd_init(m_pHideDetHandle))
            {
                dlog_info("遮挡侦测算法初始化成功");

                /*
                 * 输入码流可能不是算法固定的 1024x576。
                 * 创建算法坐标系缩放帧，运行时根据实际输入尺寸决定是否使用。
                 */
                memset_s(&m_stScaleFrameInfo, sizeof(ot_video_frame_info), 0, sizeof(ot_video_frame_info));
                if (TD_SUCCESS !=
                    mppVgs_create_video_frame_info(m_nWidth, m_nHeight, OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420, &m_stScaleFrameInfo))
                {
                    svpOd_release(m_pHideDetHandle);
                    m_pHideDetHandle = nullptr;
                    dlog_error("遮挡侦测初始化失败-创建缩放视频帧失败");
                    return false;
                }
                m_bScaleFrameCreated = true;

                /* 判断是否需要裁剪源视频 */
                if(m_nWidth != m_stRect.nWidth || m_nHeight != m_stRect.nHeight)
                {
                    m_bIsCrop = true;
                    memset_s(&m_stDstFrameInfo, sizeof(ot_video_frame_info), 0, sizeof(ot_video_frame_info));
                    /* 创建目标视频帧 */
                    if (TD_SUCCESS != mppVgs_create_video_frame_info(m_stRect.nWidth,
                                                                     m_stRect.nHeight,
                                                                     OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                                                     &m_stDstFrameInfo))
                    {
                        mppVgs_destroy_video_frame_info(&m_stScaleFrameInfo);
                        m_bScaleFrameCreated = false;
                        m_bIsCrop = false;
                        svpOd_release(m_pHideDetHandle);
                        m_pHideDetHandle = nullptr;
                        dlog_error("遮挡侦测初始化失败-创建目标视频帧失败");
                        return false;
                    }
                }
                else
                {
                    m_bIsCrop = false;
                }
                return true;
            }
            else
            {
                svpOd_release(m_pHideDetHandle);
                m_pHideDetHandle = nullptr;
                dlog_error("遮挡侦测初始化失败");
            }
        }
    }
    return false;
}

/* 反初始化 */
bool CHideDetect::unInit()
{
    /* 销毁目标视频帧 */
    if (m_bIsCrop)
    {
        mppVgs_destroy_video_frame_info(&m_stDstFrameInfo);
        m_bIsCrop = false;
    }

    if (m_bScaleFrameCreated)
    {
        mppVgs_destroy_video_frame_info(&m_stScaleFrameInfo);
        m_bScaleFrameCreated = false;
    }

    if (m_pHideDetHandle)
    {
        svpOd_release(m_pHideDetHandle);
        m_pHideDetHandle = nullptr;
    }

    return true;
}

void CHideDetect::run()
{
    pthread_setname_np(pthread_self(), "HideDetect");

    MediaData_S stMediaData;
    int nHideNum = 0;

    /* 延时2s启动，避免初始化过快 */
    std::this_thread::sleep_for(std::chrono::seconds(2));
    while (m_bRunning.load())
    {
        if (!m_pHideDetHandle)
        {
            /* 算法未使能，不进行算法初始化 */
            if (!m_stAlgoHideDetCfg.bEnable)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }

            if (!init())
            {
                dlog_error("等待遮挡侦测初始化");
                /* 延迟等待 1s */
                std::this_thread::sleep_for(std::chrono::seconds(1));
                if (!m_bRunning.load())
                {
                    break;
                }
                continue;
            }
        }

        /* 检测是否更改区域 */
        if (m_bNeedReInit.load())
        {
            unInit();

            if (!init())
            {
                dlog_error("实时区域重初始化失败");
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
            m_bNeedReInit.store(false);
            dlog_info("实时区域已更新: %dx%d @(%d,%d)",
                m_stRect.nWidth, m_stRect.nHeight, m_stRect.nX, m_stRect.nY);
        }

        /* 阻塞获取 */
        if(!m_dateQueue.pop(stMediaData, TIMEOUT_1000_MS) || stMediaData.pVideoFrameInfo == nullptr)
        {
            continue;
        }

        /* 直接使用 stMediaData.pVideoFrameInfo，避免内存拷贝 */
        ot_video_frame_info *pSrcFrameInfo = stMediaData.pVideoFrameInfo.get();
        if (!pSrcFrameInfo)
        {
            dlog_error("原始数据帧为空");
            continue;
        }

 /*
         * 先把实际输入帧缩放到算法坐标系，再按配置区域裁剪。
         * 避免全屏区域时将 1920x1080 原始帧直接送入只接受
         * 1024x576 的遮挡检测句柄。
         */
         ot_video_frame_info *pPreparedFrameInfo = pSrcFrameInfo;
         const int nSrcWidth = pSrcFrameInfo->video_frame.width;
         const int nSrcHeight = pSrcFrameInfo->video_frame.height;
         if (nSrcWidth != m_nWidth || nSrcHeight != m_nHeight)
         {
             if (!m_bScaleFrameCreated ||
                 TD_SUCCESS != mppVgs_scale(pSrcFrameInfo, &m_stScaleFrameInfo))
             {
                 dlog_error("遮挡侦测缩放失败: [%d,%d] -> [%d,%d]",
                            nSrcWidth, nSrcHeight, m_nWidth, m_nHeight);
                 continue;
             }
             pPreparedFrameInfo = &m_stScaleFrameInfo;
         }

        /* 视频帧指针，执行需要送算法的视频帧 */
        // ot_video_frame_info *pFrameInfo = pSrcFrameInfo;
        ot_video_frame_info *pFrameInfo = pPreparedFrameInfo;
        if (m_bIsCrop)
        {
            ot_rect stRect;
            stRect.x = m_stRect.nX;
            stRect.y = m_stRect.nY;
            stRect.width = m_stRect.nWidth;
            stRect.height = m_stRect.nHeight;
            /* VGS crop裁剪 */
            // if (TD_SUCCESS != mppVgs_crop(pSrcFrameInfo, &m_stDstFrameInfo, &stRect))
            if (TD_SUCCESS != mppVgs_crop(pPreparedFrameInfo, &m_stDstFrameInfo, &stRect))
            {
                /* shared_ptr会自动处理pSrcFrameInfo的释放 */
                continue;
            }
            pFrameInfo = &m_stDstFrameInfo;
        }

        /* 送分析 */
        nHideNum = m_pHideDetHandle->svpOd_sendFrame(m_pHideDetHandle, pFrameInfo);
        SEventProcessContext stCtx;
        stCtx.nChnId = stMediaData.stMediaParam.nChannel;
        stCtx.llTimestamp = TimeUtils_NS::get_currentTimestampMs();
        stCtx.pFrameInfo = pFrameInfo;
        /* 遮挡侦测后处理函数 */
        processHideDetect(nHideNum, stCtx);
    }
}

bool CHideDetect::processHideDetect(int nHideResult, const SEventProcessContext &stCtx)
{
    /* 是否报警 */
    bool bIsAlarm = false;

    int nThreshold = nOcclusionThreshold[m_stAlgoHideDetCfg.nSensitivity];
    /* 判断检测结果是否超过遮挡报警阈值 */
    if (nHideResult >= nThreshold)
    {
        bIsAlarm = true;
    }

    EventTriggerContext_S stContext;
    stContext.enEventType = Event::Type_E::OCCLUSION_DETECT;
    stContext.nChnId = stCtx.nChnId;
    stContext.llTimestamp = stCtx.llTimestamp;
#ifdef ENABLE_TVSDK_SRC
    /* perf: 有TVSDK客户端订阅时才软件编码全景图，无订阅者或冷却期跳过编码 */
    if (bIsAlarm && m_hideAlarmStateMachine.canStartAlarm() && stCtx.pFrameInfo != nullptr &&
        AiAppCommon::tvsdk_event_image_required())
    {
        auto pPayload = std::make_shared<EventTvSdkPayload_S>();
        pPayload->enType = get_tvsdk_payload_type(stContext.enEventType);
        if (AiAppCommon::encode_video_frame_to_jpeg_memory(stCtx.pFrameInfo, pPayload->stPanoramaImage) == OK)
        {
            stContext.pTvSdkPayload = pPayload;
        }
    }
#endif
    m_hideAlarmStateMachine.handleAlarmState(bIsAlarm, stContext);

    return bIsAlarm;
}