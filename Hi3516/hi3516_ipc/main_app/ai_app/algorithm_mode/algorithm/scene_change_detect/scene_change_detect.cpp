/**
 * @FilePath     : scene_change_detect.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-10 11:28:50
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-15 10:46:58
 * @Description  : 场景变更侦测
 */

#include "scene_change_detect.hpp"
#include "video_frame_jpeg_encoder.hpp"

/* 数据队列 */
#define QUEUE_MAX (2)

CSceneChangeDetect::CSceneChangeDetect()
    : m_dateQueue(QUEUE_MAX)
{
    /* 启动线程 */
    m_bRunning.store(true);
    m_thread = std::thread(&CSceneChangeDetect::run, this);
}

CSceneChangeDetect::~CSceneChangeDetect()
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

void CSceneChangeDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_stSceneChangeDetCfg.bEnable || !m_pSceneChangeDetHandle)
    {
        return;
    }

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("场景变更侦测-数据队列满了 [%d]", m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

void CSceneChangeDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stSceneChangeDetCfg.bEnable = stAlgoConfig.nEnSceneChange;

    if (m_stSceneChangeDetCfg.bEnable)
    {
        Alarm::SceneChange_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
}

void CSceneChangeDetect::setAlgoParamCfg(const Alarm::SceneChange_S &stAlgoCfg)
{
    dlog_debug("ai_app: 设置场景变更侦测参数");
    m_stSceneChangeDetCfg = stAlgoCfg;
}

bool CSceneChangeDetect::init()
{
    if (!m_pSceneChangeDetHandle)
    {
        HiMdNeedParam_S stNeedParam;
        stNeedParam.nChn = SCENE_CHANGE_DETECT_CHN;
        stNeedParam.u32Width = m_nWidth;
        stNeedParam.u32Height = m_nHeight;
        m_pSceneChangeDetHandle = svpMd_alloc(stNeedParam);
        if (m_pSceneChangeDetHandle)
        {
            // 参考帧手动更新模式
            m_pSceneChangeDetHandle->stExParam.bUpdateRef = TD_TRUE;
            if (TD_SUCCESS == m_pSceneChangeDetHandle->svpMd_init(m_pSceneChangeDetHandle))
            {
                dlog_info("场景变更侦测初始化成功");
                return true;
            }
            else
            {
                svpMd_release(m_pSceneChangeDetHandle);
                m_pSceneChangeDetHandle = nullptr;
                dlog_error("场景变更侦测初始化失败");
            }
        }
    }
    return false;
}

/* 反初始化 */
bool CSceneChangeDetect::unInit()
{
    if (m_pSceneChangeDetHandle)
    {
        svpMd_release(m_pSceneChangeDetHandle);
        m_pSceneChangeDetHandle = nullptr;
    }

    return true;
}

bool CSceneChangeDetect::reboot()
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

void CSceneChangeDetect::run()
{
    pthread_setname_np(pthread_self(), "SceneDetect");

    /* 媒体信息 */
    MediaData_S stMediaData;

    while (m_bRunning.load())
    {
        if (!m_pSceneChangeDetHandle)
        {
            if (!init())
            {
                dlog_error("等待场景变更侦测初始化");
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

        /* 直接使用 stMediaData.pVideoFrameInfo，避免内存拷贝 */
        ot_video_frame_info *pFrameInfo = stMediaData.pVideoFrameInfo.get();
        if (!pFrameInfo)
        {
            dlog_error("原始数据帧为空");
            continue;
        }

        ot_svp_dst_mem_info *pResult = NULL;
        ot_sample_svp_rect_info stRectInfo;
        /* 场景变更侦测 */
        if (m_pSceneChangeDetHandle->svpMd_sendFrame(m_pSceneChangeDetHandle, pFrameInfo, &pResult) == TD_SUCCESS)
        {
            /* 获取检测结果 */
            if (m_pSceneChangeDetHandle->svpMd_getResult(m_pSceneChangeDetHandle, &stRectInfo) == TD_SUCCESS)
            {
                SEventProcessContext stCtx;
                stCtx.nChnId = stMediaData.stMediaParam.nChannel;
                stCtx.llTimestamp = TimeUtils_NS::get_currentTimestampMs();
                stCtx.pFrameInfo = pFrameInfo;
                /* 调用场景变更侦测后处理函数 */
                processSceneChangeMode(stRectInfo, stCtx);
            }
        }

        /* 场景变更参考帧手动更新 */
        if (m_UpdateManager.handleEvent(m_pSceneChangeDetHandle->stNeedParam.nChn))
        {
            m_pSceneChangeDetHandle->svpMd_updateRef(m_pSceneChangeDetHandle, pFrameInfo);
        }
    }
}

void CSceneChangeDetect::processSceneChangeMode(ot_sample_svp_rect_info &stRectInfo, const SEventProcessContext &stCtx)
{
    /* 处理检测到的矩形区域 */
    if (!access("testPrint", F_OK))
    {
        /* 打印输出数据 */
        printResult(stRectInfo);
    }

    /* 是否报警 */
    bool bIsAlarm = false;
    /* 灵敏度判断：将用户配置的[0,100]转换为[0,1]进行比较 */
    float fSensitivityThreshold = 1 - m_stSceneChangeDetCfg.nSensitivity / 100.0f;
    /*  检查是否满足报警触发条件 */
    if (stRectInfo.sensitivity > fSensitivityThreshold)
    {
        bIsAlarm = true;
        if (!access("testPrint", F_OK))
        {
            dlog_debug("[场景变更侦测] 灵敏度: %f > %f", stRectInfo.sensitivity, fSensitivityThreshold);
            /* 发送结果至OSD模块，进行框选显示 */
            send_detectionResult_to_osd(m_nWidth, m_nHeight, stRectInfo);
        }
    }

    /* 判断是否报警 */
    EventTriggerContext_S stContext;
    stContext.enEventType = Event::Type_E::SCENE_CHANGE;
    stContext.nChnId = stCtx.nChnId;
    stContext.llTimestamp = stCtx.llTimestamp;
#ifdef ENABLE_TVSDK_SRC
    /* perf: 有TVSDK客户端订阅时才软件编码全景图，无订阅者或冷却期跳过编码 */
    if (bIsAlarm && m_sceneChangeAlarmStateMachine.canStartAlarm() && stCtx.pFrameInfo != nullptr &&
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
    m_sceneChangeAlarmStateMachine.handleAlarmState(bIsAlarm, stContext);
}
