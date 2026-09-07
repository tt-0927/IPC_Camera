/**
 * @FilePath     : hide_detect_logic.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-25 11:49:38
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-25 13:50:00
 * @Description  : 遮挡侦测事件判断逻辑实现
 */

#include "hide_detect_logic.hpp"

#include "share_data.h"
#include "video_frame_jpeg_encoder.hpp"

/* 遮挡灵敏度档位（与 LEGACY hide_detect.hpp 定义一致） */
enum class EOcclusionSens : int
{
    OFF = 0,
    LOW = 1,
    MID = 2,
    HIGH = 3,
    COUNT = 4 /* 边界检查用 */
};

/* 遮挡报警阈值（与 LEGACY hide_detect.hpp 定义一致） */
static const int nOcclusionThreshold[static_cast<int>(EOcclusionSens::COUNT)] = {
64,  /* OFF */
20,  /* LOW */
17,  /* MID */
15   /* HIGH */
};

CHideDetectLogic::CHideDetectLogic()
{
    /* 默认侦测区域（与 LEGACY 构造函数一致） */
    m_stRect.nX = 0;
    m_stRect.nY = 0;
    m_stRect.nWidth = m_nWidth;
    m_stRect.nHeight = m_nHeight;
}

bool CHideDetectLogic::apply_config(const Alarm::HideAlarm_S &stAlgoCfg)
{
    std::lock_guard<std::mutex> lock(m_configMutex);

    dlog_debug("ai_app: 设置遮挡侦测参数");
    m_stAlgoHideDetCfg = stAlgoCfg;

    convert_resolution_and_enable_unlocked(m_stAlgoHideDetCfg);

    /* 用户设定的区域（已转换至算法坐标系） */
    Common::Rect_S stUser = m_stAlgoHideDetCfg.stRect;

    bool bNeedReinit = false;
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

        /* 宽高变化 => 重初始化（位置变化不更新，保持 LEGACY 既有行为） */
        if ((m_stRect.nWidth != stUser.nWidth) || (m_stRect.nHeight != stUser.nHeight))
        {
            m_stRect = stUser;
            bNeedReinit = true;
        }
    }

    return bNeedReinit;
}

void CHideDetectLogic::set_enabled(bool bEnable)
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    m_stAlgoHideDetCfg.bEnable = bEnable;
}

bool CHideDetectLogic::is_enabled() const
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_stAlgoHideDetCfg.bEnable;
}

Common::Rect_S CHideDetectLogic::rect() const
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_stRect;
}

bool CHideDetectLogic::is_crop() const
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    return is_crop_unlocked();
}

bool CHideDetectLogic::process_result(int nHideResult, const SEventProcessContext &stCtx)
{
    std::lock_guard<std::mutex> lock(m_configMutex);

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

void CHideDetectLogic::convert_resolution_and_enable_unlocked(Alarm::HideAlarm_S &stConfig)
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

bool CHideDetectLogic::is_crop_unlocked() const
{
    /* 与 LEGACY init() 中 m_bIsCrop 判定一致：区域尺寸与算法分辨率不一致时需要裁剪 */
    return (m_nWidth != m_stRect.nWidth) || (m_nHeight != m_stRect.nHeight);
}
