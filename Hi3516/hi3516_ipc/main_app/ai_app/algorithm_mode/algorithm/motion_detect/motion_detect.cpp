/**
 * @FilePath     : motion_detect.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-10 11:28:50
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-15 10:46:47
 * @Description  : 移动侦测
 */

#include "motion_detect.hpp"
#include "isp_control.h"
#include "isp_dayNight.h"
#include "video_frame_jpeg_encoder.hpp"

/* 数据队列 */
#define QUEUE_MAX (2)

CMotionDetect::CMotionDetect()
    : m_dateQueue(QUEUE_MAX)
{
    /* 默认侦测区域 */
    m_stRect.nX = 0;
    m_stRect.nY = 0;
    m_stRect.nWidth = m_nWidth;
    m_stRect.nHeight = m_nHeight;

    /* 启动线程 */
    m_bRunning.store(true);
    m_thread = std::thread(&CMotionDetect::run, this);
}

CMotionDetect::~CMotionDetect()
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

void CMotionDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_stMotionDetCfg.bEnable || !m_pMotionDetHandle || !m_bIsDraw)
    {
        return;
    }

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("移动侦测-数据队列满了 [%d]", m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

void CMotionDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stMotionDetCfg.bEnable = stAlgoConfig.nEnMotionDetect;

    if (m_stMotionDetCfg.bEnable)
    {
        Alarm::MotionDetection_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
}

void CMotionDetect::setAlgoParamCfg(const Alarm::MotionDetection_S &stAlgoCfg)
{
    dlog_debug("ai_app: 设置移动侦测参数");

    /* 是否需要重新初始化 */
    bool bReboot = false;
    /* 切换模式，需要重新初始化 */
    if (m_stMotionDetCfg.enMode != stAlgoCfg.enMode)
    {
        bReboot = true;
    }

    m_stMotionDetCfg = stAlgoCfg;

    if(m_stMotionDetCfg.enMode == Alarm::MotionType_E::MOTION_NORMAL) // 普通模式
    {
        if(m_stMotionDetCfg.stMotionNormalMode.nRegionType) // 网格
        {
            if (!std::holds_alternative<Alarm::MotionNormalMode_S::AreaGrid>(m_stMotionDetCfg.stMotionNormalMode.varRegion))
            {
                dlog_error("[移动侦测] 网格区域类型与配置数据不匹配");
                m_stMotionDetCfg.bEnable = false;
                m_bIsDraw = false;
                goto reboot;
            }

            /* 网格二维向量 */
            auto &grid = std::get<Alarm::MotionNormalMode_S::AreaGrid>(m_stMotionDetCfg.stMotionNormalMode.varRegion);
            if (grid.size() < GRID_HEIGHT_DEFAULT)
            {
                dlog_error("[移动侦测] 网格区域行数错误");
                m_stMotionDetCfg.bEnable = false;
                m_bIsDraw = false;
                goto reboot;
            }
            for (const auto &row : grid)
            {
                if (row.size() < GRID_WIDTH_DEFAULT)
                {
                    dlog_error("[移动侦测] 网格区域列数错误");
                    m_stMotionDetCfg.bEnable = false;
                    m_bIsDraw = false;
                    goto reboot;
                }
            }

            Common::Rect_S stRect;
            /* 转换网格区域至矩形数据结构 */
            convert_gridRegion_to_rect(grid, m_nWidth, m_nHeight, stRect);
            /* 必须是宏块宽的偶数倍，宽度需16字节向下对齐，防止超限 */
            stRect.nX = ALIGN_BACK(stRect.nX, 16);
            stRect.nY = ALIGN_BACK(stRect.nY, 4);
            stRect.nWidth = ALIGN_BACK(stRect.nWidth, 16);
            stRect.nHeight = ALIGN_BACK(stRect.nHeight, 4);
            dlog_debug("[移动侦测] : m_stRect: [%d,%d][%d,%d]", m_stRect.nX, m_stRect.nY, m_stRect.nWidth, m_stRect.nHeight);
            dlog_debug("[移动侦测] :   stRect: [%d,%d][%d,%d]", stRect.nX, stRect.nY, stRect.nWidth, stRect.nHeight);
            if(stRect.nWidth == 0 || stRect.nHeight == 0)
            {
                /* 未正确设置区域，不使能侦测 */
                dlog_error("[移动侦测] 网格区域未选中有效宏块，不使能侦测");
                m_stMotionDetCfg.bEnable = false;
                m_bIsDraw = false;
                goto reboot;
            }

            m_bIsDraw = true;
            /* 如果改变了侦测区域的宽高，就重启 */
            if(stRect.nWidth != m_stRect.nWidth || stRect.nHeight != m_stRect.nHeight)
            {
                bReboot = true;
            }
            m_stRect = stRect;
        }
        else // 矩形
        {
            if (!std::holds_alternative<Common::Rect_S>(m_stMotionDetCfg.stMotionNormalMode.varRegion))
            {
                dlog_error("[移动侦测] 矩形区域类型与配置数据不匹配");
                m_stMotionDetCfg.bEnable = false;
                m_bIsDraw = false;
                goto reboot;
            }

            Common::Rect_S stRect = std::get<Common::Rect_S>(m_stMotionDetCfg.stMotionNormalMode.varRegion);
            if (!stRect.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight))
            {
                m_stMotionDetCfg.bEnable = false;
                m_bIsDraw = false;
                goto reboot;
            }

            /* 必须是宏块宽的偶数倍，宽度需16字节向下对齐，防止超限 */
            stRect.nX = ALIGN_BACK(stRect.nX, 16);
            stRect.nY = ALIGN_BACK(stRect.nY, 4);
            stRect.nWidth = ALIGN_BACK(stRect.nWidth, 16);
            stRect.nHeight = ALIGN_BACK(stRect.nHeight, 4);
            dlog_debug("[移动侦测] : m_stRect: [%d,%d][%d,%d]", m_stRect.nX, m_stRect.nY, m_stRect.nWidth, m_stRect.nHeight);
            dlog_debug("[移动侦测] :   stRect: [%d,%d][%d,%d]", stRect.nX, stRect.nY, stRect.nWidth, stRect.nHeight);
            if(stRect.nWidth == 0 || stRect.nHeight == 0)
            {
                /* 未正确设置区域，不使能侦测 */
                dlog_error("[移动侦测] 矩形区域为空，不使能侦测");
                m_stMotionDetCfg.bEnable = false;
                m_bIsDraw = false;
                goto reboot;
            }

            m_bIsDraw = true;
            /* 如果改变了侦测区域的宽高，就重启 */
            if(stRect.nWidth != m_stRect.nWidth || stRect.nHeight != m_stRect.nHeight)
            {
                bReboot = true;
            }
            m_stRect = stRect;
        }
    }
    else if(m_stMotionDetCfg.enMode == Alarm::MotionType_E::MOTION_EXPERT) // 专家模式
    {
        m_stRect.nX = 0;
        m_stRect.nY = 0;
        m_stRect.nWidth = m_nWidth;
        m_stRect.nHeight = m_nHeight;
        m_bIsDraw = false;
        if (!m_stMotionDetCfg.stMotionExpertMode.vstMotionRegion.empty())
        {
            /* 转换区域坐标分辨率至算法分辨率 */
            for (auto &rule : m_stMotionDetCfg.stMotionExpertMode.vstMotionRegion)
            {
                rule.stRect.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight);
                if (rule.stRect.nWidth != 0 && rule.stRect.nHeight != 0)
                {
                    m_bIsDraw = true;
                }
            }
        }

        /* 未正确设置区域，不使能侦测 */
        if (!m_bIsDraw)
        {
            m_stMotionDetCfg.bEnable = false;
        }
    }

reboot:
    /* 重新初始化 */
    if (bReboot)
    {
        reboot();
    }
}

bool CMotionDetect::init()
{
    if (!m_pMotionDetHandle)
    {
        HiMdNeedParam_S stNeedParam;
        stNeedParam.nChn = MOTION_DETECT_CHN;
        dlog_debug("[移动侦测] 分辨率: [%d,%d]", m_stRect.nWidth, m_stRect.nHeight);
        stNeedParam.u32Width = m_stRect.nWidth;
        stNeedParam.u32Height = m_stRect.nHeight;
        m_pMotionDetHandle = svpMd_alloc(stNeedParam);
        if (m_pMotionDetHandle)
        {
            /* 调整Sad阈值，使移动侦测更灵敏 */
            #if CAP_IO_EXTERNAL_DDR_00S
            m_pMotionDetHandle->stExParam.u16SadThreshold = 35;
            #else
            m_pMotionDetHandle->stExParam.u16SadThreshold = 25;
            #endif
            if (TD_SUCCESS == m_pMotionDetHandle->svpMd_init(m_pMotionDetHandle))
            {
                dlog_info("移动侦测初始化成功");

                /*
                 * 输入码流分辨率可能不是算法固定的 1024x576。
                 * 预先创建全尺寸缩放帧，运行时按实际输入尺寸决定是否使用。
                 */
                memset_s(&m_stScaleFrameInfo, sizeof(ot_video_frame_info), 0, sizeof(ot_video_frame_info));
                if (TD_SUCCESS !=
                    mppVgs_create_video_frame_info(m_nWidth, m_nHeight, OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420, &m_stScaleFrameInfo))
                {
                    svpMd_release(m_pMotionDetHandle);
                    m_pMotionDetHandle = nullptr;
                    dlog_error("移动侦测初始化失败-创建缩放视频帧失败");
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
                        svpMd_release(m_pMotionDetHandle);
                        m_pMotionDetHandle = nullptr;
                        dlog_error("移动侦测初始化失败-创建目标视频帧失败");
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
                mppVgs_destroy_video_frame_info(&m_stScaleFrameInfo);
                m_bScaleFrameCreated = false;
                m_bIsCrop = false;
                svpMd_release(m_pMotionDetHandle);
                m_pMotionDetHandle = nullptr;
                dlog_error("移动侦测初始化失败");
            }
        }
    }
    return false;
}

/* 反初始化 */
bool CMotionDetect::unInit()
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

    if (m_pMotionDetHandle)
    {
        svpMd_release(m_pMotionDetHandle);
        m_pMotionDetHandle = nullptr;
    }

    return true;
}

bool CMotionDetect::reboot()
{
    std::lock_guard<std::mutex> lock(m_mutex);

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

void CMotionDetect::run()
{
    pthread_setname_np(pthread_self(), "MotionDetect");

    /* 媒体信息 */
    MediaData_S stMediaData;

    /* 延时4s启动，避免首次检测误报 */
    sleep(4);
    while (m_bRunning.load())
    {
        if (!m_pMotionDetHandle)
        {
            /* 没有算法使能，不进行算法初始化 */
            if (!m_stMotionDetCfg.bEnable)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }

            if (!init())
            {
                dlog_error("等待移动侦测初始化");
                /* 延迟等待 1s */
                std::this_thread::sleep_for(std::chrono::seconds(1));
                if (!m_bRunning.load())
                {
                    break;
                }
                continue;
            }
        }

        /* 阻塞获取 */
        if(!m_dateQueue.pop(stMediaData, TIMEOUT_1000_MS) || stMediaData.pVideoFrameInfo == nullptr)
        {
            continue;
        }

        std::lock_guard<std::mutex> lock(m_mutex);

        /* 直接使用 stMediaData.pVideoFrameInfo，避免内存拷贝 */
        ot_video_frame_info *pSrcFrameInfo = stMediaData.pVideoFrameInfo.get();
        if (!pSrcFrameInfo)
        {
            dlog_error("原始数据帧为空");
            continue;
        }
       /*
         * 先按实际输入帧尺寸缩放到算法坐标系，再按配置区域裁剪。
         * 旧逻辑只比较算法尺寸和区域尺寸，全屏区域时会把 1920x1080
         * 原始帧直接送入只接受 1024x576 的 MD 句柄。
         */
         ot_video_frame_info *pPreparedFrameInfo = pSrcFrameInfo;
         const int nSrcWidth = pSrcFrameInfo->video_frame.width;
         const int nSrcHeight = pSrcFrameInfo->video_frame.height;
         if (nSrcWidth != m_nWidth || nSrcHeight != m_nHeight)
         {
             if (!m_bScaleFrameCreated ||
                 TD_SUCCESS != mppVgs_scale(pSrcFrameInfo, &m_stScaleFrameInfo))
             {
                 dlog_error("移动侦测缩放失败: [%d,%d] -> [%d,%d]",
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

        /* 构建事件处理上下文 */
        SEventProcessContext stCtx;
        stCtx.nChnId = stMediaData.stMediaParam.nChannel;
        stCtx.llTimestamp = TimeUtils_NS::get_currentTimestampMs();
        stCtx.pFrameInfo = pFrameInfo;

        ot_svp_dst_mem_info *pResult = NULL;
        ot_sample_svp_rect_info stRectInfo;
        /* 移动侦测 */
        if (m_pMotionDetHandle->svpMd_sendFrame(m_pMotionDetHandle, pFrameInfo, &pResult) == TD_SUCCESS)
        {
            /* 获取检测结果 */
            if (m_pMotionDetHandle->svpMd_getResult(m_pMotionDetHandle, &stRectInfo) == TD_SUCCESS)
            {
                if (m_stMotionDetCfg.bEnable)
                {
                    if(m_stMotionDetCfg.enMode == 0)    /* 普通模式 */
                    {
                        /* 调用普通模式处理函数 */
                        processNormalMode(stRectInfo, stCtx);
                    }
                    else if(m_stMotionDetCfg.enMode == 1)   /* 专家模式 */
                    {
                        /* 调用专家模式处理函数 */
                        processExpertMode(stRectInfo, stCtx);
                    }
                }
            }
        }
    }
}

bool CMotionDetect::isDaytime() const
{
    if (m_stMotionDetCfg.stMotionExpertMode.nExpertDayNightCtrl == 0)
    {
        /* 关闭日夜切换，始终返回false（使用关闭时的灵敏度） */
        return false;
    }
    else if (m_stMotionDetCfg.stMotionExpertMode.nExpertDayNightCtrl == 1)
    {
        /* 自动切换 - 这里需要根据光线传感器或其他硬件信息判断 */
        /* 获取白天黑夜状态 */
        return !CDayNightController::instance()->isNightMode();
    }
    else if (m_stMotionDetCfg.stMotionExpertMode.nExpertDayNightCtrl == 2)
    {
        /* 定时切换 */
        /*自当天开始的秒数*/
        int nCurrentTime = TimeUtils_NS::getSecondsSinceStartOfDay();
        /* 将开始时间和结束时间转换为秒 */
        int nStartTime = m_stMotionDetCfg.stMotionExpertMode.stDayTime.stStart.nHour * 3600 + m_stMotionDetCfg.stMotionExpertMode.stDayTime.stStart.nMinute * 60 + m_stMotionDetCfg.stMotionExpertMode.stDayTime.stStart.nSecond;
        int nEndTime = m_stMotionDetCfg.stMotionExpertMode.stDayTime.stStop.nHour * 3600 + m_stMotionDetCfg.stMotionExpertMode.stDayTime.stStop.nMinute * 60 + m_stMotionDetCfg.stMotionExpertMode.stDayTime.stStop.nSecond;

        return (nCurrentTime >= nStartTime && nCurrentTime <= nEndTime);
    }

    return false;
}

int CMotionDetect::calculateOverlapArea(const Common::Rect_S &rect1, const Common::Rect_S &rect2) const
{
    int left = std::max(rect1.nX, rect2.nX);
    int top = std::max(rect1.nY, rect2.nY);
    int right = std::min(rect1.nX + rect1.nWidth, rect2.nX + rect2.nWidth);
    int bottom = std::min(rect1.nY + rect1.nHeight, rect2.nY + rect2.nHeight);

    if (left < right && top < bottom)
    {
        return (right - left) * (bottom - top);
    }

    return OK;
}

Common::Rect_S CMotionDetect::convertToRect(const ot_sample_svp_rect &rect) const
{
    Common::Rect_S result;

    /* point[0]是左上角，point[2]是右下角 */
    int minX = std::min({ rect.point[0].x, rect.point[1].x, rect.point[2].x, rect.point[3].x });
    int maxX = std::max({ rect.point[0].x, rect.point[1].x, rect.point[2].x, rect.point[3].x });
    int minY = std::min({ rect.point[0].y, rect.point[1].y, rect.point[2].y, rect.point[3].y });
    int maxY = std::max({ rect.point[0].y, rect.point[1].y, rect.point[2].y, rect.point[3].y });

    result.nX = minX;
    result.nY = minY;
    result.nWidth = maxX - minX;
    result.nHeight = maxY - minY;

    return result;
}

void CMotionDetect::processNormalMode(ot_sample_svp_rect_info &stRectInfo,
                                      const SEventProcessContext &stCtx)
{
    /* 处理检测到的矩形区域 */
    if (m_bIsCrop) /* 判断是否需要换算结果坐标至算法默认分辨率 */
    {
        for (int j = 0; j < stRectInfo.num; j++)
        {
            stRectInfo.rect[j].point[0].x += m_stRect.nX;
            stRectInfo.rect[j].point[0].y += m_stRect.nY;
            stRectInfo.rect[j].point[1].x += m_stRect.nX;
            stRectInfo.rect[j].point[1].y += m_stRect.nY;
            stRectInfo.rect[j].point[2].x += m_stRect.nX;
            stRectInfo.rect[j].point[2].y += m_stRect.nY;
            stRectInfo.rect[j].point[3].x += m_stRect.nX;
            stRectInfo.rect[j].point[3].y += m_stRect.nY;
        }
    }
    /* 打印输出数据 */
    if (!access("testPrint", F_OK))
    {
        printResult(stRectInfo);
    }

    /* 是否报警 */
    bool bIsAlarm = false;

    /* 灵敏度判断：将用户配置的[0,100]转换为[0,1]进行比较 */
    float fSensitivityThreshold = 1 - m_stMotionDetCfg.stMotionNormalMode.nSensitivity / 100.0f;
    /*  检查是否满足报警触发条件 */
    if (stRectInfo.sensitivity > fSensitivityThreshold)
    {
        bIsAlarm = true;
        if (!access("testPrint", F_OK))
        {
            dlog_debug("[移动侦测] 灵敏度: %f > %f", stRectInfo.sensitivity, fSensitivityThreshold);
        }
        /* 动态分析 */
        if (m_stMotionDetCfg.bDynamicAnalysisEnable)
        {
            /* 发送结果至OSD模块，进行框选显示 */
            send_detectionResult_to_osd(m_nWidth, m_nHeight, stRectInfo);
        }
    }

    /* 构建事件触发上下文 */
    EventTriggerContext_S stContext;
    stContext.enEventType = Event::Type_E::MOTION_DETECT;
    stContext.nChnId = stCtx.nChnId;
    stContext.llTimestamp = stCtx.llTimestamp;
#ifdef ENABLE_TVSDK_SRC
    /* perf: 有TVSDK客户端订阅时才软件编码全景图，无订阅者跳过编码 */
    if (bIsAlarm && m_motionAlarmStateMachine.canStartAlarm() && stCtx.pFrameInfo != nullptr &&
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
    m_motionAlarmStateMachine.handleAlarmState(bIsAlarm, stContext);
}

void CMotionDetect::processExpertMode(ot_sample_svp_rect_info &stRectInfo,
                                      const SEventProcessContext &stCtx)
{
    /* 打印输出数据 */
    if (!access("testPrint", F_OK))
    {
        printResult(stRectInfo);
    }

    /* 判断当前是白天还是夜晚 true：白天 false：夜晚 */
    bool bIsDaytime = isDaytime();

    /* 存储每个配置区域的触发状态 */
    std::vector<bool> vbRegionTriggered(m_stMotionDetCfg.stMotionExpertMode.vstMotionRegion.size(), false);
    std::vector<float> vfRegionSensitivity(m_stMotionDetCfg.stMotionExpertMode.vstMotionRegion.size(), 0.0f);

    if (!m_stMotionDetCfg.stMotionExpertMode.vstMotionRegion.empty())
    {
        /* 遍历每个配置的侦测区域 */
        for (size_t configIdx = 0; configIdx < m_stMotionDetCfg.stMotionExpertMode.vstMotionRegion.size(); configIdx++)
        {
            const auto &configRegion = m_stMotionDetCfg.stMotionExpertMode.vstMotionRegion[configIdx];

            /* 获取当前应使用的灵敏度阈值 */
            float fSensitivityThreshold;
            if (m_stMotionDetCfg.stMotionExpertMode.nExpertDayNightCtrl == 0)
            {
                /* 关闭日夜切换，使用关闭时的灵敏度 */
                fSensitivityThreshold = 1.0f - configRegion.nCloseSensitivity / 100.0f;
            }
            else if (bIsDaytime)
            {
                /* 白天灵敏度 */
                fSensitivityThreshold = 1.0f - configRegion.nDaytimeSensitivity / 100.0f;
            }
            else
            {
                /* 夜晚灵敏度 */
                fSensitivityThreshold = 1.0f - configRegion.nNightSensitivity / 100.0f;
            }

            /* 计算配置区域面积 */
            int nConfigAreaSize = configRegion.stRect.nWidth * configRegion.stRect.nHeight;
            if (nConfigAreaSize <= 0)
            {
                continue;
            }

            /* 计算该配置区域内的移动面积 */
            int nTotalOverlapArea = 0;

            /* 遍历算法输出的所有移动区域 */
            for (size_t i = 0; i < stRectInfo.num; i++)
            {
                /* 将算法输出的点坐标转换为矩形 */
                Common::Rect_S motionRect = convertToRect(stRectInfo.rect[i]);

                /* 计算与配置区域的重叠面积 */
                int nOverlapArea = calculateOverlapArea(configRegion.stRect, motionRect);
                nTotalOverlapArea += nOverlapArea;
            }

            /* 计算该配置区域的灵敏度（移动面积占配置区域面积的比例） */
            float fRegionSensitivityRatio = static_cast<float>(nTotalOverlapArea) / static_cast<float>(nConfigAreaSize);
            vfRegionSensitivity[configIdx] = fRegionSensitivityRatio;

            /* 判断是否触发报警 */
            if (fRegionSensitivityRatio > fSensitivityThreshold)
            {
                vbRegionTriggered[configIdx] = true;
                dlog_debug("[移动侦测 专家模式] 区域[%d] 触发灵敏度[%.3f] > [%.3f] 日夜模式:[%s]",
                           configRegion.nAreaNo,
                           fRegionSensitivityRatio,
                           fSensitivityThreshold,
                           bIsDaytime ? "白天" : "夜晚");
            }
        }
    }

    /* 检查是否有任何区域触发报警 */
    bool anyRegionTriggered = false;
    for (bool triggered : vbRegionTriggered)
    {
        if (triggered)
        {
            anyRegionTriggered = true;
            break;
        }
    }

    if (anyRegionTriggered)
    {
        /* 动态分析 */
        if (m_stMotionDetCfg.bDynamicAnalysisEnable)
        {
            /* 发送结果至OSD模块，进行框选显示 */
            /* 这里可以选择显示所有触发的区域，或者只显示最大的区域 */
            send_detectionResult_to_osd(m_nWidth, m_nHeight, stRectInfo);
        }
    }

    /* 构建事件触发上下文 */
    EventTriggerContext_S stContext;
    stContext.enEventType = Event::Type_E::MOTION_DETECT;
    stContext.nChnId = stCtx.nChnId;
    stContext.llTimestamp = stCtx.llTimestamp;
#ifdef ENABLE_TVSDK_SRC
    /* perf: 有TVSDK客户端订阅时才软件编码全景图，无订阅者跳过编码 */
    if (anyRegionTriggered && m_motionAlarmStateMachine.canStartAlarm() && stCtx.pFrameInfo != nullptr &&
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
    m_motionAlarmStateMachine.handleAlarmState(anyRegionTriggered, stContext);
}
