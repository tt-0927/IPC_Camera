/**
 * @FilePath     : motion_detect_model.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-25 11:49:38
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-25 11:49:38
 * @Description  : 移动侦测模型适配器实现
 */

#include "motion_detect_model.hpp"

#include "event_configure.h"
#include "time_utils.h"
#include "video_define.h"

const DetectionModelDescriptor_S CMotionDetectModel::ms_stDescriptor = {
    "MotionDetect",          /* 模型名 */
    PIXEL_WIDTH_1024,        /* 算法输入分辨率宽 */
    PIXEL_HEIGHT_576,        /* 算法输入分辨率高 */
    true,                    /* 需要按配置区域裁剪 */
    0,                       /* 同帧执行顺序 */
    4000                     /* 启动延时：与 LEGACY sleep(4) 防首次误报一致 */
};

const DetectionModelDescriptor_S &CMotionDetectModel::descriptor() const
{
    return ms_stDescriptor;
}

bool CMotionDetectModel::isEnabled() const
{
    /* 与 LEGACY recvMediaData 门控一致：总开关 + 区域已绘制 */
    return m_logic.is_enabled();
}

bool CMotionDetectModel::shouldProcess(int nChannelId)
{
    /* 帧率控制：每 500ms 放行一帧（与 LEGACY recvMediaData 的 EventManager 一致） */
    if (!m_recvManager.handleEvent(nChannelId))
    {
        return false;
    }
    /* 重建请求未处理完时跳过本帧，避免新区域配置送入旧句柄 */
    return !m_bNeedReinit.load();
}

int CMotionDetectModel::process(const ot_video_frame_info *pPreparedFrame,
                                const MediaData_S &stMediaData)
{
    /* 仅 Worker 线程调用 */
    if (pPreparedFrame == nullptr || m_pMotionDetHandle == nullptr)
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

    ot_svp_dst_mem_info *pResult = nullptr;
    ot_sample_svp_rect_info stRectInfo;
    ot_video_frame_info *pFrameInfo = const_cast<ot_video_frame_info *>(pPreparedFrame);
    /* 移动侦测（与 LEGACY run() 语义一致） */
    if (m_pMotionDetHandle->svpMd_sendFrame(m_pMotionDetHandle, pFrameInfo, &pResult) != TD_SUCCESS)
    {
        return ERR_SEND;
    }
    if (m_pMotionDetHandle->svpMd_getResult(m_pMotionDetHandle, &stRectInfo) != TD_SUCCESS)
    {
        return ERR;
    }
    /* 事件判断（按模式分发、坐标补偿、灵敏度判断、报警状态机） */
    m_logic.process_result(stRectInfo, stCtx);
    return OK;
}

int CMotionDetectModel::init()
{
    /* 仅 Worker 线程调用（引擎保证 init/unInit/process 同线程串行） */
    if (m_pMotionDetHandle != nullptr)
    {
        return OK;
    }

    /* 消费重建请求：初始化期间到达的新配置会重新置位，由引擎下一轮再次重建 */
    m_bNeedReinit.store(false);

    const Common::Rect_S stRect = m_logic.rect();
    HiMdNeedParam_S stNeedParam;
    stNeedParam.nChn = MOTION_DETECT_CHN;
    dlog_debug("[移动侦测] 分辨率: [%d,%d]", stRect.nWidth, stRect.nHeight);
    stNeedParam.u32Width = static_cast<td_u32>(stRect.nWidth);
    stNeedParam.u32Height = static_cast<td_u32>(stRect.nHeight);
    m_pMotionDetHandle = svpMd_alloc(stNeedParam);
    if (m_pMotionDetHandle == nullptr)
    {
        dlog_error("移动侦测分配句柄失败");
        return ERR;
    }

    /* 调整Sad阈值，使移动侦测更灵敏（与 LEGACY init 一致） */
#if CAP_IO_EXTERNAL_DDR_00S
    m_pMotionDetHandle->stExParam.u16SadThreshold = 35;
#else
    m_pMotionDetHandle->stExParam.u16SadThreshold = 25;
#endif
    if (TD_SUCCESS != m_pMotionDetHandle->svpMd_init(m_pMotionDetHandle))
    {
        svpMd_release(m_pMotionDetHandle);
        m_pMotionDetHandle = nullptr;
        dlog_error("移动侦测初始化失败");
        return ERR;
    }
    dlog_info("移动侦测初始化成功");
    return OK;
}

void CMotionDetectModel::unInit()
{
    /* 仅 Worker 线程调用 */
    if (m_pMotionDetHandle != nullptr)
    {
        svpMd_release(m_pMotionDetHandle);
        m_pMotionDetHandle = nullptr;
    }
}

void CMotionDetectModel::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    /* 配置线程调用（逻辑内部互斥保护），语义与 LEGACY setAlgoEnCfg 一致 */
    if (stAlgoConfig.nEnMotionDetect != 0)
    {
        Alarm::MotionDetection_S stInfo;
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

bool CMotionDetectModel::needsReinit() const
{
    return m_bNeedReinit.load();
}

const Common::Rect_S *CMotionDetectModel::crop_rect() const
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
