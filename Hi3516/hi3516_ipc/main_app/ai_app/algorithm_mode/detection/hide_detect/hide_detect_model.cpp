/**
 * @FilePath     : hide_detect_model.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-25 11:49:38
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-25 13:50:00
 * @Description  : 遮挡侦测模型适配器实现
 */

#include "hide_detect_model.hpp"

#include "event_configure.h"
#include "time_utils.h"
#include "video_define.h"

const DetectionModelDescriptor_S CHideDetectModel::ms_stDescriptor = {
    "HideDetect",            /* 模型名 */
    PIXEL_WIDTH_1024,        /* 算法输入分辨率宽 */
    PIXEL_HEIGHT_576,        /* 算法输入分辨率高 */
    true,                    /* 需要按配置区域裁剪 */
    1,                       /* 同帧执行顺序（在移动侦测之后） */
    2000                     /* 启动延时：与 LEGACY sleep(2) 防初始化过快一致 */
};

const DetectionModelDescriptor_S &CHideDetectModel::descriptor() const
{
    return ms_stDescriptor;
}

bool CHideDetectModel::isEnabled() const
{
    return m_logic.is_enabled();
}

bool CHideDetectModel::shouldProcess(int nChannelId)
{
    /* 帧率控制：每 500ms 放行一帧（与 LEGACY recvMediaData 的 EventManager 一致） */
    if (!m_recvManager.handleEvent(nChannelId))
    {
        return false;
    }
    /* 重建请求未处理完时跳过本帧，避免新区域配置送入旧句柄 */
    return !m_bNeedReinit.load();
}

int CHideDetectModel::process(const ot_video_frame_info *pPreparedFrame,
                                   const MediaData_S &stMediaData)
{
    /* 仅 Worker 线程调用 */
    if (pPreparedFrame == nullptr || m_pHideDetHandle == nullptr)
    {
        return ERR_PARAM;
    }
    /* 配置在分发间隙更新：本帧跳过，等待引擎下一轮重建句柄 */
    if (m_bNeedReinit.load())
    {
        return OK;
    }

    /* 构建事件处理上下文 */
    SEventProcessContext stCtx;
    stCtx.nChnId = stMediaData.stMediaParam.nChannel;
    stCtx.llTimestamp = TimeUtils_NS::get_currentTimestampMs();
    stCtx.pFrameInfo = const_cast<ot_video_frame_info *>(pPreparedFrame);

    /* 送分析（与 LEGACY run() 语义一致：sendFrame 返回遮挡数量） */
    int nHideNum = m_pHideDetHandle->svpOd_sendFrame(m_pHideDetHandle,
                                                     const_cast<ot_video_frame_info *>(pPreparedFrame));
    /* 遮挡侦测后处理（阈值判断 + 报警状态机） */
    m_logic.process_result(nHideNum, stCtx);
    return OK;
}

int CHideDetectModel::init()
{
    /* 仅 Worker 线程调用（引擎保证 init/unInit/process 同线程串行） */
    if (m_pHideDetHandle != nullptr)
    {
        return OK;
    }

    /* 消费重建请求：初始化期间到达的新配置会重新置位，由引擎下一轮再次重建 */
    m_bNeedReinit.store(false);

    const Common::Rect_S stRect = m_logic.rect();
    HiOdNeedParam_S stNeedParam;
    stNeedParam.nWidth = static_cast<td_u32>(stRect.nWidth);
    stNeedParam.nHeight = static_cast<td_u32>(stRect.nHeight);
    m_pHideDetHandle = svpOd_alloc(stNeedParam);
    if (m_pHideDetHandle == nullptr)
    {
        dlog_error("遮挡侦测分配句柄失败");
        return ERR;
    }
    if (TD_SUCCESS != m_pHideDetHandle->svpOd_init(m_pHideDetHandle))
    {
        svpOd_release(m_pHideDetHandle);
        m_pHideDetHandle = nullptr;
        dlog_error("遮挡侦测初始化失败");
        return ERR;
    }
    dlog_info("遮挡侦测算法初始化成功");
    return OK;
}

void CHideDetectModel::unInit()
{
    /* 仅 Worker 线程调用 */
    if (m_pHideDetHandle != nullptr)
    {
        svpOd_release(m_pHideDetHandle);
        m_pHideDetHandle = nullptr;
    }
}

void CHideDetectModel::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    /* 配置线程调用（逻辑内部互斥保护），语义与 LEGACY setAlgoEnCfg 一致 */
    if (stAlgoConfig.nEnOcclusionDetect != 0)
    {
        Alarm::HideAlarm_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        if (m_logic.apply_config(stInfo))
        {
            m_bNeedReinit.store(true);
        }
    }
    else
    {
        m_logic.set_enabled(false);
    }
}

bool CHideDetectModel::needsReinit() const
{
    return m_bNeedReinit.load();
}

const Common::Rect_S *CHideDetectModel::crop_rect() const
{
    /* 与 LEGACY init 中 m_bIsCrop 判定一致：区域尺寸与算法分辨率不一致时返回裁剪区域 */
    if (!m_logic.is_crop())
    {
        return nullptr;
    }
    /* 快照到成员：引擎在同帧内立即使用，避免返回逻辑内部指针 */
    m_stCropSnapshot = m_logic.rect();
    return &m_stCropSnapshot;
}
