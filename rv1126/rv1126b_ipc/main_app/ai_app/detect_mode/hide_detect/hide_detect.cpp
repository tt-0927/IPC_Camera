/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-11-05 10:38:00
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2026-07-22 16:42:14
 * @FilePath: /1126/rv1126b_ipc/main_app/ai_app/algorithm_mode/algorithm/motion_detect/hide_detect.cpp
 * @Description: 遮挡侦测
 */

#include "hide_detect.hpp"
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
    return ;
}

void CHideDetect::setAlgoParamCfg(const Alarm::HideAlarm_S &stAlgoCfg)
{
    dlog_debug("ai_app: 设置遮挡侦测参数");
    m_stAlgoHideDetCfg = stAlgoCfg;

    convertResolutionAndEnable(m_stAlgoHideDetCfg);

    /* 用户设定的区域 */
    Common::Rect_S stUser = m_stAlgoHideDetCfg.stRect;

    /* 对齐 IVE 要求 */
    // stUser.nX      = ALIGN_BACK(stUser.nX, 2);
    // stUser.nY      = ALIGN_BACK(stUser.nY, 2);
    // stUser.nWidth  = ALIGN_BACK(stUser.nWidth, 2);
    // stUser.nHeight = ALIGN_BACK(stUser.nHeight, 2);
    stUser.nX   &= ~1;
    stUser.nY   &= ~1;
    stUser.nWidth  &= ~1;
    stUser.nHeight &= ~1;

    dlog_debug("[遮挡侦测] : m_stRect: [%d,%d][%d,%d]", m_stRect.nX, m_stRect.nY, m_stRect.nWidth, m_stRect.nHeight);
    dlog_debug("[遮挡侦测] :   stUser: [%d,%d][%d,%d]", stUser.nX, stUser.nY, stUser.nWidth, stUser.nHeight);

    /* 宽高变化 => 重初始化 */
    if ((m_stRect.nWidth  != stUser.nWidth) || (m_stRect.nHeight != stUser.nHeight))
    {
        m_stRect = stUser;
        m_bNeedReInit.store(true);
    }

    return ;
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

    return ;
}

bool CHideDetect::init()
{
    if (!m_pHideDetHandle)
    {      
        // CameraObstruction_NS::InParam_S stNeedParam;
        // stNeedParam.nWidth = m_stRect.nWidth;
        // stNeedParam.nHeight = m_stRect.nHeight;
        CameraObstruction_NS::InParam_S stInParam;
        stInParam.bDebug = false;
        m_pHideDetHandle = new CameraObstruction_NS::CCameraObstructionV1_0(stInParam);
        
        if (m_pHideDetHandle)
        {
            if (m_pHideDetHandle->init())
            {
                dlog_info("遮挡侦测算法初始化成功");
                /* 判断是否需要裁剪源视频 */
                if(m_nWidth != m_stRect.nWidth || m_nHeight != m_stRect.nHeight)
                {
                    m_bIsCrop = true;
                }
                else
                {
                    m_bIsCrop = false;
                }
                return true;
            }
            else
            {
                // svpOd_release(m_pHideDetHandle);
                delete m_pHideDetHandle;
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
    // if (m_bIsCrop)
    // {
    //     mppVgs_destroy_video_frame_info(&m_stDstFrameInfo);
    // }

    if (m_pHideDetHandle)
    {
        // svpOd_release(m_pHideDetHandle);
        delete m_pHideDetHandle;
        m_pHideDetHandle = nullptr;
    }

    return true;
}

void CHideDetect::run()
{
    pthread_setname_np(pthread_self(), "HideDetect");

    MediaData_S stMediaData;
    
    while (m_bRunning.load())
    {
        if (!m_pHideDetHandle)
        {
            if (!init())
            {
                dlog_error("等待遮挡侦测初始化");
                /* 延迟等待 1s */
                std::unique_lock<std::mutex> lock(m_mutex);
                m_condition.wait_for(lock, std::chrono::seconds(1), [this] {
                    return !m_bRunning.load();
                });
            }
            continue;
        }

        /* 检测是否更改区域并重初始化 */
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
            dlog_info("实时区域已更新: %dx%d @(%d,%d)", m_stRect.nWidth, m_stRect.nHeight, m_stRect.nX, m_stRect.nY);
        }

        /* 阻塞获取数据 */
        if(!m_dateQueue.pop(stMediaData, 100) || stMediaData.pData == nullptr) 
        {
            continue;
        }

        if (!stMediaData.pData.get() || stMediaData.nSize == 0)
        {
            dlog_error("原始数据帧为空");
            continue;
        }

        if(m_stRect.nWidth <= 0 || m_stRect.nHeight <= 0) 
        { 
            dlog_debug("设置检测区域有误");
            continue;
        } 

        int sw = stMediaData.stMediaParam.nVideoWidth;
        int sh = stMediaData.stMediaParam.nVideoHeight;
        
        int cx = 0, cy = 0, cw = sw, ch = sh;
        int dw = sw, dh = sh;

        // 处理裁剪
        if (m_bIsCrop)
        {
            // 特定坐标转换
            int raw_cx = sw - m_stRect.nX - m_stRect.nWidth;
            int raw_cy = sh - m_stRect.nY - m_stRect.nHeight;
            int raw_cw = m_stRect.nWidth;
            int raw_ch = m_stRect.nHeight;
            // 强制对齐修正
            cx = (raw_cx >> 1) << 1;
            cy = (raw_cy >> 1) << 1;
            // cw 必须是 4 的倍数 (为了满足 RGB888 目标步长 4 字节对齐要求)
            cw = (raw_cw >> 2) << 2;
            // ch 建议是偶数
            ch = (raw_ch >> 1) << 1;
            // 安全校验保护 , 防止对齐后越界) 
            if (cx < 0) cx = 0;
            if (cy < 0) cy = 0;
            // 确保限制后的宽度依然是 4 的倍数
            if (cx + cw > sw) cw = (sw - cx) & (~3); 
            // 确保限制后的高度依然是偶数
            if (cy + ch > sh) ch = (sh - cy) & (~1);

            dw = cw;
            dh = ch;
        }
        else
        {
            // 宽度 4 对齐校验
            dw = (sw >> 2) << 2;
            dh = (sh >> 1) << 1;
        }

        // 预分配/复用目标内存
        if (dw > 0 && dh > 0) 
        {
            m_fullRgbMat.create(dh, dw, CV_8UC3);
        } 
        else 
        {
            dlog_error("计算得到的有效宽度或高度为0，请检查裁剪参数");
            continue; 
        }

        // 调用 RGA 硬件处理转换
        bool rga_ok = rga_image_transform(
            stMediaData.pData.get(), sw, sh, RK_FORMAT_YCbCr_420_SP, // 源: NV12
            m_fullRgbMat.data, dw, dh, RK_FORMAT_RGB_888,           // 目标: RGB
            cx, cy, cw, ch,                                                            // 裁剪参数
            0                                                                // 旋转
        );

        // RGA 失败，回退CPU 处理
        if (!rga_ok)
        {
            dlog_warn("遮挡侦测 RGA 失败，回退至 CPU 处理");
            uint8_t* pRaw = reinterpret_cast<uint8_t*>(stMediaData.pData.get());
            if (m_bIsCrop)
            {
                cv::Mat yFull(sh, sw, CV_8UC1, pRaw);
                cv::Mat uvFull(sh / 2, sw / 2, CV_8UC2, pRaw + (sw * sh));

                // 定义 Y 平面的 ROI
                cv::Rect yRoi(cx, cy, cw, ch);
                
                // 定义 UV 平面的 ROI (坐标和尺寸都要除以 2)
                // 在 NV12 中，UV 是交错存储的单平面，当作 CV_8UC2 处理时，宽度不需要额外处理，直接 /2
                cv::Rect uvRoi(cx / 2, cy / 2, cw / 2, ch / 2);

                try {
                    cv::cvtColorTwoPlane(yFull(yRoi), uvFull(uvRoi), m_fullRgbMat, cv::COLOR_YUV2RGB_NV12);
                } catch (const cv::Exception& e) {
                    dlog_error("Fallback cvtColorTwoPlane error: %s", e.what());
                }
            }
            else
            {
                cv::Mat nv12(sh * 3 / 2, sw, CV_8UC1, pRaw);
                cv::cvtColor(nv12, m_fullRgbMat, cv::COLOR_YUV2RGB_NV12);
            }
        }

        /* 视频帧指针，执行需要送算法的视频帧 */
        CameraObstruction_NS::InData_S stInData;
        stInData.inMat = m_fullRgbMat;

        // /* Debug 保存图片 */
        // if (access("/debugHideImage", F_OK) == 0)
        // {
        //     static uint64_t lastSaveTime = 0;
        //     uint64_t now = time(nullptr);
        //     if (now - lastSaveTime > 100) { // 控制保存频率
        //         std::string filename = "/mnt/algo/hide/hide_" + std::to_string(now) + ".jpg";
        //         cv::imwrite(filename, stInData.inMat);
        //         lastSaveTime = now;
        //     }
        // }

        if (stInData.inMat.empty()) 
        {
            dlog_error("ai_app: 遮挡侦测输入图片为空");
            continue;
        }

        stInData.stParam.dThres = nOcclusionThreshold[m_stAlgoHideDetCfg.nSensitivity]; 

        /* 送分析 */
        CameraObstruction_NS::Result_S stOutData;
        if (!m_pHideDetHandle->process(stInData, stOutData))
        {
            dlog_error("遮挡侦测推理失败");
        }
        
        /* 遮挡侦测后处理函数 */
        processHideDetect(stOutData.bBlockFlag, stMediaData);
    }
}

bool CHideDetect::processHideDetect(bool bIsAlarm, const MediaData_S &stMediaData) 
{
    /* 上报事件 */
#ifdef ENABLE_TVSDK_SRC
    EventTriggerContext_S stContext;
    stContext.enEventType = Event::Type_E::OCCLUSION_DETECT;
    stContext.nChnId = stMediaData.stMediaParam.nChannel;
    stContext.llTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch())
                               .count();

    if (bIsAlarm && !m_fullRgbMat.empty())
    {
        auto pPayload = std::make_shared<EventTvSdkPayload_S>();
        pPayload->enType = get_tvsdk_payload_type(stContext.enEventType);
        if (encode_mat_to_tvsdk_image(m_fullRgbMat, pPayload->stPanoramaImage))
        {
            stContext.pTvSdkPayload = pPayload;
        }
    }

    m_hideAlarmStateMachine.handleAlarmState(bIsAlarm, stContext);
#else
    (void)stMediaData;
    m_hideAlarmStateMachine.handleAlarmState(bIsAlarm, Event::Type_E::OCCLUSION_DETECT);
#endif

    return bIsAlarm;
}