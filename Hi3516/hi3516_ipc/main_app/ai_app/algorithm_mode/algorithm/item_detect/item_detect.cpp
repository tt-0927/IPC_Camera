/**
 * @FilePath     : item_detect.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-07-29 20:10:37
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-04 06:25:07
 * @Description  : 物品侦测
 */

#include "item_detect.hpp"
#include "video_frame_jpeg_encoder.hpp"
#include "time_utils.h"

/* 数据队列 */
#define QUEUE_MAX (2)

#if 1
namespace
{
#if CAP_EXHIBITION_OSD_PANEL
/**
 * @brief   : 将 LD 灵敏度结果转换为面板展示用置信度
 * @param    {const HiLdRegionResult_S &} stResult：区域检测结果
 * @return   {int} 百分比置信度
 * @note    : svp_ld 仅输出 u32Sensitivity=100-(S1/ST)*100，数值越小表示变化越明显，
 *            面板这里按 100-u32Sensitivity 反算成更直观的“变化置信度”百分比。
 */
int get_item_confidence_percent(const HiLdRegionResult_S &stResult)
{
    const int nConfidencePercent = 100 - static_cast<int>(stResult.u32Sensitivity);
    return std::max(0, std::min(100, nConfidencePercent));
}

/**
 * @brief   : 构造物品遗留/拿取事件的展会面板条目
 * @param    {int} nRegionIndex：区域下标
 * @param    {const Alarm::Region_S &} stRegion：区域定义
 * @param    {const HiLdRegionResult_S &} stResult：区域检测结果
 * @param    {bool} bAlarm：当前区域是否已报警
 * @return   {OsdPanel::PanelItem_S} 面板条目
 */
OsdPanel::PanelItem_S build_item_panel_item(int nRegionIndex,
                                            const Alarm::Region_S &stRegion,
                                            const HiLdRegionResult_S &stResult,
                                            bool bAlarm)
{
    OsdPanel::PanelItem_S stItem;
    if (nRegionIndex < 0 || !stRegion.IsValid() || !stResult.bValid)
    {
        return stItem;
    }

    const int nConfidencePercent = get_item_confidence_percent(stResult);

    stItem.clear();
    stItem.strTitle = "区域 " + std::to_string(nRegionIndex + 1);
    stItem.bAlarm = bAlarm;
    stItem.bHasRect = true;
    stItem.stRect = to_exhibition_panel_rect(stRegion);
    stItem.nSortKey = nRegionIndex + 1;
    stItem.nPriority = build_exhibition_panel_priority(bAlarm, static_cast<float>(nConfidencePercent) / 100.0f);
    stItem.vecFields = {{"置信度", get_exhibition_panel_percent_text(nConfidencePercent)}};
    return stItem;
}
#endif
} // namespace

CItemDetect::CItemDetect()
    : m_dateQueue(QUEUE_MAX)
{
    /* 启动线程 */
    m_bRunning.store(true);
    m_thread = std::thread(&CItemDetect::run, this);
}

CItemDetect::~CItemDetect()
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

void CItemDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_stUnattendedObjectDetCfg.bEnable && !m_stObjectRemovalDetCfg.bEnable)
    {
        return;
    }

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("物品侦测-数据队列满了 [%d]", m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

void CItemDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stUnattendedObjectDetCfg.bEnable = stAlgoConfig.nEnUnattendedObject;
    m_stObjectRemovalDetCfg.bEnable = stAlgoConfig.nEnObjectRemoval;

    bool bRegionChange = false;
    if (m_stUnattendedObjectDetCfg.bEnable)
    {
        Alarm::UnattendedObject_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        /* 转换坐标再比较 */
        auto stconvertResolutionInfo = stInfo;
        convertResolutionAndEnable(stconvertResolutionInfo);
        /* 判断坐标是否变化 */
        for (size_t i = 0; i < stconvertResolutionInfo.aRule.size() && i < m_stUnattendedObjectDetCfg.aRule.size(); i++)
        {
            auto &newRule = stconvertResolutionInfo.aRule[i];
            auto &oldRule = m_stUnattendedObjectDetCfg.aRule[i];

            if (newRule.stRegion != oldRule.stRegion)
            {
                bRegionChange = true;
            }
        }
        setAlgoParamCfg(stInfo);
    }
    if (m_stObjectRemovalDetCfg.bEnable)
    {
        Alarm::ObjectRemoval_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        /* 转换坐标再比较 */
        auto stconvertResolutionInfo = stInfo;
        convertResolutionAndEnable(stconvertResolutionInfo);
        /* 判断坐标是否变化 */
        for (size_t i = 0; i < stconvertResolutionInfo.aRule.size() && i < m_stObjectRemovalDetCfg.aRule.size(); i++)
        {
            auto &newRule = stconvertResolutionInfo.aRule[i];
            auto &oldRule = m_stObjectRemovalDetCfg.aRule[i];

            if (newRule.stRegion != oldRule.stRegion)
            {
                bRegionChange = true;
            }
        }
        setAlgoParamCfg(stInfo);
    }
    /* 重新初始化物品侦测 */
    if (m_stUnattendedObjectDetCfg.bEnable || m_stObjectRemovalDetCfg.bEnable)
    {
        /* 已经初始化过物品侦测，区域有变化 */
        if (m_pItemDetHandle && bRegionChange)
        {
            reboot();
        }
    }
}

void CItemDetect::setAlgoParamCfg(const Alarm::UnattendedObject_S &stAlgoCfg)
{
    dlog_debug("ai_app: 设置物品遗留侦测参数");
    m_stUnattendedObjectDetCfg = stAlgoCfg;

    /* 转换区域坐标并判断有无正确的多边形区域，是否使能算法 */
    convertResolutionAndEnable(m_stUnattendedObjectDetCfg);
}

void CItemDetect::setAlgoParamCfg(const Alarm::ObjectRemoval_S &stAlgoCfg)
{
    dlog_debug("ai_app: 设置物品拿取侦测参数");
    m_stObjectRemovalDetCfg = stAlgoCfg;

    /* 转换区域坐标并判断有无正确的多边形区域，是否使能算法 */
    convertResolutionAndEnable(m_stObjectRemovalDetCfg);
}

bool CItemDetect::init()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_pItemDetHandle)
    {
        HiLdNeedParam_S stNeedParam;
        memset_s(&stNeedParam, sizeof(HiLdNeedParam_S), 0, sizeof(HiLdNeedParam_S));
        stNeedParam.u32Width = m_nWidth;
        stNeedParam.u32Height = m_nHeight;
        /* svp_ld 检测区域先放入物品遗留检测区域，再物品拿取检测区域 */
        for (size_t i = 0; i < m_stUnattendedObjectDetCfg.aRule.size(); i++)
        {
            /* 当前区域物品遗留检测规则 */
            auto &rule = m_stUnattendedObjectDetCfg.aRule[i];
            /* 当前 svp_ld 检测区域 */
            auto &regions = stNeedParam.stRegions[i];
            /* 物品遗留检测区域 */
            if (rule.stRegion.IsValid())
            {
                regions.bEnable = TD_TRUE;
                regions.u32PointNum = rule.stRegion.nPointNum;
                for (td_u32 j = 0; j < regions.u32PointNum; j++)
                {
                    regions.aPoints[j].fX = rule.stRegion.aPoint[j].fX;
                    regions.aPoints[j].fY = rule.stRegion.aPoint[j].fY;
                }
                stNeedParam.u32RegionNum++;
            }
            else
            {
                regions.bEnable = TD_FALSE;
            }
        }

        /* 计算物品拿取检测区域的起始索引 */
        int nRemovalStartIndex = m_stUnattendedObjectDetCfg.bEnable ? UNATTENDED_OBJECT_DETECT_REGION_DEFAULT : 0;
        /* 物品拿取检测区域 */
        for (size_t i = 0; i < m_stObjectRemovalDetCfg.aRule.size(); i++)
        {
            /* 当前区域物品拿取检测规则 */
            auto &rule = m_stObjectRemovalDetCfg.aRule[i];
            int nIndex = i + nRemovalStartIndex;
            /* 当前 svp_ld 检测区域 */
            auto &regions = stNeedParam.stRegions[nIndex];
            if (rule.stRegion.IsValid())
            {
                regions.bEnable = TD_TRUE;
                regions.u32PointNum = rule.stRegion.nPointNum;
                for (td_u32 j = 0; j < regions.u32PointNum; j++)
                {
                    regions.aPoints[j].fX = rule.stRegion.aPoint[j].fX;
                    regions.aPoints[j].fY = rule.stRegion.aPoint[j].fY;
                }
                stNeedParam.u32RegionNum++;
            }
            else
            {
                regions.bEnable = TD_FALSE;
            }
        }

        m_pItemDetHandle = svpLd_alloc(stNeedParam);
        if (!m_pItemDetHandle)
        {
            dlog_error("物品侦测初始化失败");
            return false;
        }

        m_pItemDetHandle->stExParam.bManualUpdate = TD_TRUE;
        if (TD_SUCCESS == m_pItemDetHandle->svpLd_init(m_pItemDetHandle))
        {
            dlog_info("物品侦测初始化成功");
            memset_s(&m_stDstFrameInfo, sizeof(ot_video_frame_info), 0, sizeof(ot_video_frame_info));
            /* 创建目标视频帧 */
            if (TD_SUCCESS == mppVgs_create_video_frame_info(m_nWidth,
                                                             m_nHeight,
                                                             OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                                             &m_stDstFrameInfo))
            {
                return true;
            }
            dlog_error("物品侦测初始化失败-创建目标视频帧失败");
        }

        svpLd_release(m_pItemDetHandle);
        m_pItemDetHandle = nullptr;
        dlog_error("物品侦测初始化失败");
    }

    return false;
}

/* 反初始化 */
bool CItemDetect::unInit()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    /* 销毁目标视频帧 */
    if(m_stDstFrameInfo.pool_id != OT_VB_INVALID_POOL_ID)
    {
        mppVgs_destroy_video_frame_info(&m_stDstFrameInfo);
    }

    if (m_pItemDetHandle)
    {
        svpLd_release(m_pItemDetHandle);
        m_pItemDetHandle = nullptr;
    }

    return true;
}

bool CItemDetect::reboot()
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

void CItemDetect::run()
{
    pthread_setname_np(pthread_self(), "ItemDetect");

    /* 媒体信息 */
    MediaData_S stMediaData;
    /* 下次参考帧更新的时间戳 */
    long long llNextUpdateTime = 0;

    /* 延时4s启动 */
    std::this_thread::sleep_for(std::chrono::seconds(4));
    while (m_bRunning.load())
    {
        if (!m_pItemDetHandle)
        {
            /* 没有算法使能，不进行算法初始化 */
            if (!m_stUnattendedObjectDetCfg.bEnable && !m_stObjectRemovalDetCfg.bEnable)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }

            if (!init())
            {
                dlog_error("等待物品侦测初始化");
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
        ot_video_frame_info *pSrcFrameInfo = stMediaData.pVideoFrameInfo.get();
        if (!pSrcFrameInfo)
        {
            dlog_error("原始数据帧为空");
            continue;
        }

        /* 是否需要缩放源视频 */
        bool bIsScale = false;

        /* 判断是否需要缩放源视频 */
        if (m_nWidth != stMediaData.stMediaParam.nVideoWidth || m_nHeight != stMediaData.stMediaParam.nVideoHeight)
        {
            bIsScale = true;
        }

        /* 锁住互斥量，准备处理 */
        std::lock_guard<std::mutex> lock(m_mutex);

        /* 视频帧指针，执行需要送算法的视频帧 */
        ot_video_frame_info *pFrameInfo = pSrcFrameInfo;
        if (bIsScale)
        {
            /* VGS scale缩放 */
            if (TD_SUCCESS != mppVgs_scale(pSrcFrameInfo, &m_stDstFrameInfo))
            {
                /* shared_ptr会自动处理pSrcFrameInfo的释放 */
                continue;
            }
            pFrameInfo = &m_stDstFrameInfo;
        }

        /* 标记是否需要延迟参考帧更新（报警中或触发检测中） */
        bool bDelayRefUpdate = false;
        /* 标记是否需要强制立即更新参考帧（强制停止后） */
        bool bForceRefUpdate = false;
        /* 物品侦测 */
        if (m_pItemDetHandle->svpLd_sendFrame(m_pItemDetHandle, pFrameInfo) == TD_SUCCESS)
        {
            HiLdRegionResult_S stResults[SVP_LD_MAX_REGION_NUM];
            /* 获取检测结果 */
            if (m_pItemDetHandle->svpLd_getResult(m_pItemDetHandle, stResults, SVP_LD_MAX_REGION_NUM) == TD_SUCCESS)
            {
                if (!access("testPrint", F_OK))
                {
                    /* 打印输出数据 */
                    m_pItemDetHandle->svpLd_printResult(m_pItemDetHandle);
                }
                /* OSD 动态分析显示数组 */
                std::vector<Common::RectInfo_S> vstRectInfo;
                /* 构造事件处理上下文 */
                SEventProcessContext stCtx;
                stCtx.nChnId = stMediaData.stMediaParam.nChannel;
                stCtx.llTimestamp = TimeUtils_NS::get_currentTimestampMs();
                stCtx.pFrameInfo = pFrameInfo;
                /* 物品遗留侦测 */
                if (m_stUnattendedObjectDetCfg.bEnable)
                {
#if CAP_EXHIBITION_OSD_PANEL
                    OsdPanel::PanelFrame_S stUnattendedPanelFrame;
#endif
                    // double time = time_get_ms();
                    /* 物品遗留侦测后处理函数 */
                    bDelayRefUpdate |= processUnattendedObjectDetect(stResults,
                                                                    vstRectInfo,
                                                                    bForceRefUpdate,
                                                                    stCtx
#if CAP_EXHIBITION_OSD_PANEL
                                                                    , &stUnattendedPanelFrame
#endif
                    );
                    // dlog_debug("物品遗留侦测后处理函数耗时：%f %d", time_get_ms() - time, bEventAlarm);
#if CAP_EXHIBITION_OSD_PANEL
                    send_panelResult_to_osd(stUnattendedPanelFrame);
#endif
                }
                /* 物品拿取侦测 */
                if (m_stObjectRemovalDetCfg.bEnable)
                {
#if CAP_EXHIBITION_OSD_PANEL
                    OsdPanel::PanelFrame_S stObjectRemovalPanelFrame;
#endif
                    // double time = time_get_ms();
                    /* 物品拿取侦测后处理函数 */
                    bDelayRefUpdate |= processObjectRemovalDetect(stResults,
                                                                  vstRectInfo,
                                                                  bForceRefUpdate,
                                                                  stCtx
#if CAP_EXHIBITION_OSD_PANEL
                                                                  , &stObjectRemovalPanelFrame
#endif
                    );
                    // dlog_debug("物品拿取侦测后处理函数耗时：%f %d", time_get_ms() - time, bEventAlarm);
#if CAP_EXHIBITION_OSD_PANEL
                    send_panelResult_to_osd(stObjectRemovalPanelFrame);
#endif
                }
                if (!vstRectInfo.empty())
                {
                    /* 发送结果至OSD模块，进行框选显示 */
                    send_detectionResult_to_osd(m_nWidth, m_nHeight, vstRectInfo);
                }
            }
        }

        /* 获取当前时间 */
        long long llCurrentTime = TimeUtils_NS::get_currentTimestampS();

        /* 参考帧更新逻辑优化 */
        if (bForceRefUpdate || llCurrentTime >= llNextUpdateTime)
        {
            /**
             * 触发场景：
             * 1. 到达10秒周期
             * 2. 状态机检测到由于长时间触发导致的“强制结束”，必须更新参考帧来“吃掉”当前背景中的异物
             */
            m_pItemDetHandle->svpLd_updateRef(m_pItemDetHandle, pFrameInfo);
            llNextUpdateTime = llCurrentTime + REFERENCE_FRAME_UPDATE_FREQUENCY;
            dlog_debug("参考帧已更新 %s", bForceRefUpdate ? "[强制触发]" : "[周期触发]");
        }
        else if (bDelayRefUpdate)
        {
            /** 
             * 触发场景：有区域正在报警或灵敏度达标且未超时
             * 检查距离下次更新的时间，若不足3秒则延长
             */
            long long llRemainTime = llNextUpdateTime - llCurrentTime;
            if (llRemainTime < EVENT_END_TIME_THRESHOLD)
            {
                llNextUpdateTime = llCurrentTime + EVENT_END_TIME_THRESHOLD;
            }
        }
    }
}

// info /*----------------------- 算法后处理 -----------------------*/
bool CItemDetect::processUnattendedObjectDetect(HiLdRegionResult_S *pstResult,
                                                std::vector<Common::RectInfo_S> &vstRectInfo,
                                                bool &bForceRefUpdate,
                                                const SEventProcessContext &stCtx
#if CAP_EXHIBITION_OSD_PANEL
                                                , OsdPanel::PanelFrame_S *pstPanelFrame
#endif
)
{
    if (pstResult == nullptr)
    {
        return false;
    }

#if CAP_EXHIBITION_OSD_PANEL
    prepare_exhibition_panel_frame(pstPanelFrame, Event::Type_E::UNATTENDED_OBJECT, m_nWidth, m_nHeight);
#endif

    /* 获取当前时间 */
    long long llCurrentTime = TimeUtils_NS::get_currentTimestampS();

    /* 标记本轮检测是否有任何区域触发了告警（达到时间阈值） */
    bool bAnyRegionAlarm = false;
    /* 标记本轮检测是否需要延迟参考帧更新 */
    bool bDelayRefUpdate = false;
    /* 标记是否发送区域至OSD动态分析数组中 */
    static bool s_bOsdRegion = false;

    /* 判断每个区域 */
    for (size_t i = 0; i < m_stUnattendedObjectDetCfg.aRule.size() && i < SVP_LD_MAX_REGION_NUM; i++)
    {
        /* 跳过无效区域 */
        if (!m_stUnattendedObjectDetCfg.aRule[i].stRegion.IsValid())
        {
            continue;
        }

        /* 当前区域是否满足触发条件（灵敏度达标） */
        bool bCurrentTriggered = false;
        /* 当前区域灵敏度结果 */
        auto &result = pstResult[i];
        /* 当前区域物品遗留检测规则 */
        auto &rule = m_stUnattendedObjectDetCfg.aRule[i];
        /* 当前区域触发状态 */
        auto &triggerState = m_astRegionTriggerState[i];

        /* 结果是否有效 */
        if (result.bValid)
        {
            /* 判断灵敏度 */
            if (result.u32Sensitivity <= rule.nSensitivity)
            {
                bCurrentTriggered = true;
            }
        }

        /* 状态机处理 */
        if (bCurrentTriggered)
        {
#if CAP_EXHIBITION_OSD_PANEL
            upsert_exhibition_panel_item(pstPanelFrame,
                                         build_item_panel_item(static_cast<int>(i),
                                                               rule.stRegion,
                                                               result,
                                                               false));
#endif
            /* 只要有灵敏度达标，就请求主循环延迟参考帧更新，防止在倒计时期间目标被吸收到背景中 */
            bDelayRefUpdate = true;

            /* 当前帧满足触发条件 */
            if (!triggerState.bTriggering)
            {
                /* 之前未触发，现在开始触发，记录开始时间 */
                triggerState.bTriggering = true;
                triggerState.llTriggerStartTime = llCurrentTime;
                s_bOsdRegion = false;
                if (!access("testPrint", F_OK))
                {
                    dlog_debug("物品遗留侦测-区域[%d] 开始触发，灵敏度 %d <= %d",
                               i,
                               result.u32Sensitivity,
                               rule.nSensitivity);
                }
            }
            else
            {
                /* 之前已经在触发中，检查是否达到时间阈值 */
                long long llTriggerDuration = llCurrentTime - triggerState.llTriggerStartTime;
                long long llTimeThreshold = rule.nTimeThreshold;

                /* 如果一直在连续触发，暂定连续触发 区域时间阈值加5秒后进行停止 */
                if (llTriggerDuration >= llTimeThreshold && llTriggerDuration < llTimeThreshold + 5)
                {
                    /* 达到时间阈值，触发真正的告警 */
                    dlog_info("物品遗留侦测-区域[%d] 连续触发 %lld 秒 >= %d 秒，灵敏度 %d <= %d，触发告警",
                              i,
                              llTriggerDuration,
                              rule.nTimeThreshold,
                              result.u32Sensitivity,
                              rule.nSensitivity);
                    bAnyRegionAlarm = true;

                    /* 请求主循环强制更新参考帧 */
                    bForceRefUpdate = true;
                    /* 强制更新时不需要延迟 */
                    bDelayRefUpdate = false;

#if CAP_EXHIBITION_OSD_PANEL
                    upsert_exhibition_panel_item(pstPanelFrame,
                                                 build_item_panel_item(static_cast<int>(i),
                                                                       rule.stRegion,
                                                                       result,
                                                                       true));
#endif

                    /* 只有事件开始时执行 */
                    if(!s_bOsdRegion)
                    {
                        /* 添加结果至动态分析数组 */
                        add_result_to_vector(rule.stRegion, vstRectInfo);
                        s_bOsdRegion = true;
                    }
                    /* 只要有一个区域达到告警条件，就可以停止遍历 */
                    break;
                }
                else if (llTriggerDuration >= llTimeThreshold + 5)
                {
                    if (!access("testPrint", F_OK))
                    {
                        dlog_debug("物品遗留侦测-区域[%d] 连续触发，持续了 %lld 秒 >（ 时间阈值 %d + 5 秒）,进行强制停止触发", i,
                                   llTriggerDuration, rule.nTimeThreshold);
                    }

                    /* 请求主循环强制更新参考帧，将当前物体学习为背景 */
                    bForceRefUpdate = true;
                    /* 强制更新时不需要延迟 */
                    bDelayRefUpdate = false;

                    /* 重置状态 */
                    triggerState.bTriggering = false;
                    triggerState.llTriggerStartTime = 0;
                    s_bOsdRegion = false;
                }
                else if (!access("testPrint", F_OK))
                {
                    dlog_debug("物品遗留侦测-区域[%d] 持续触发中，已触发 %lld 秒 / %d 秒，灵敏度 %d <= %d",
                               i,
                               llTriggerDuration,
                               rule.nTimeThreshold,
                               result.u32Sensitivity,
                               rule.nSensitivity);
                }
            }
        }
        else
        {
            /* 当前帧不满足触发条件,触发中断 */
            if (triggerState.bTriggering)
            {
                /* 之前在触发中，现在中断了，重置状态 */
                long long llTriggerDuration = llCurrentTime - triggerState.llTriggerStartTime;
                if (!access("testPrint", F_OK))
                {
                    dlog_debug("物品遗留侦测-区域[%d] 触发中断，持续了 %lld 秒（ 时间阈值 %d 秒）",
                               i,
                               llTriggerDuration,
                               rule.nTimeThreshold);
                }
                triggerState.bTriggering = false;
                triggerState.llTriggerStartTime = 0;
                s_bOsdRegion = false;
            }
        }
    }

    /* 物品遗留告警状态机处理 */
    EventTriggerContext_S stAlarmCtx;
    stAlarmCtx.enEventType = Event::Type_E::UNATTENDED_OBJECT;
    stAlarmCtx.nChnId = stCtx.nChnId;
    stAlarmCtx.llTimestamp = stCtx.llTimestamp;
#ifdef ENABLE_TVSDK_SRC
    if (bAnyRegionAlarm && stCtx.pFrameInfo != nullptr)
    {
        auto pPayload = std::make_shared<EventTvSdkPayload_S>();
        pPayload->enType = get_tvsdk_payload_type(stAlarmCtx.enEventType);
        if (AiAppCommon::encode_video_frame_to_jpeg_memory(stCtx.pFrameInfo, pPayload->stPanoramaImage) == OK)
        {
            stAlarmCtx.pTvSdkPayload = pPayload;
        }
    }
#endif
    auto state = m_unattendedObjectAlarmStateMachine.handleAlarmState(bAnyRegionAlarm, stAlarmCtx);
    if(state)
    {
        /* 如果状态机处于活跃（报警中），一定要延迟更新 */
        bDelayRefUpdate = true;
    }

    return bDelayRefUpdate;
}

bool CItemDetect::processObjectRemovalDetect(HiLdRegionResult_S *pstResult,
                                             std::vector<Common::RectInfo_S> &vstRectInfo,
                                             bool &bForceRefUpdate,
                                             const SEventProcessContext &stCtx
#if CAP_EXHIBITION_OSD_PANEL
                                             , OsdPanel::PanelFrame_S *pstPanelFrame
#endif
)
{
    if (pstResult == nullptr)
    {
        return false;
    }

#if CAP_EXHIBITION_OSD_PANEL
    prepare_exhibition_panel_frame(pstPanelFrame, Event::Type_E::OBJECT_REMOVAL, m_nWidth, m_nHeight);
#endif

    /* 获取当前时间 */
    long long llCurrentTime = TimeUtils_NS::get_currentTimestampS();

    /* 标记本轮检测是否有任何区域触发了告警（达到时间阈值） */
    bool bAnyRegionAlarm = false;
    /* 标记本轮检测是否需要延迟参考帧更新 */
    bool bDelayRefUpdate = false;
    /* 标记是否发送区域至OSD动态分析数组中 */
    static bool s_bOsdRegion = false;

    /* 计算物品拿取检测区域的起始索引 */
    int nRemovalStartIndex = m_stUnattendedObjectDetCfg.bEnable ? UNATTENDED_OBJECT_DETECT_REGION_DEFAULT : 0;

    /* 判断每个区域 */
    for (size_t i = 0; i < m_stObjectRemovalDetCfg.aRule.size(); i++)
    {
        /* 跳过无效区域 */
        if (!m_stObjectRemovalDetCfg.aRule[i].stRegion.IsValid())
        {
            continue;
        }

        /* 计算在结果数组中的实际索引 */
        int nResultIndex = nRemovalStartIndex + i;
        if (nResultIndex >= SVP_LD_MAX_REGION_NUM)
        {
            dlog_error("物品拿取侦测-区域索引越界: %d >= %d", nResultIndex, SVP_LD_MAX_REGION_NUM);
            break;
        }

        /* 当前区域是否满足触发条件（灵敏度达标） */
        bool bCurrentTriggered = false;
        /* 当前区域灵敏度结果 */
        auto &result = pstResult[nResultIndex];
        /* 当前区域物品拿取检测规则 */
        auto &rule = m_stObjectRemovalDetCfg.aRule[i];
        /* 当前区域触发状态 */
        auto &triggerState = m_astRegionTriggerState[nResultIndex];

        /* 结果是否有效 */
        if (result.bValid)
        {
            /* 判断灵敏度 */
            if (result.u32Sensitivity <= rule.nSensitivity)
            {
                bCurrentTriggered = true;
            }
        }

        /* 状态机处理 */
        if (bCurrentTriggered)
        {
#if CAP_EXHIBITION_OSD_PANEL
            upsert_exhibition_panel_item(pstPanelFrame,
                                         build_item_panel_item(static_cast<int>(i),
                                                               rule.stRegion,
                                                               result,
                                                               false));
#endif
            /* 只要有灵敏度达标，就请求主循环延迟参考帧更新，防止在倒计时期间目标被吸收到背景中 */
            bDelayRefUpdate = true;

            /* 当前帧满足触发条件 */
            if (!triggerState.bTriggering)
            {
                /* 之前未触发，现在开始触发，记录开始时间 */
                triggerState.bTriggering = true;
                triggerState.llTriggerStartTime = llCurrentTime;
                s_bOsdRegion = false;
                if (!access("testPrint", F_OK))
                {
                    dlog_debug("物品拿取侦测-区域[%d] 开始触发，灵敏度 %d <= %d", i, result.u32Sensitivity, rule.nSensitivity);
                }
            }
            else
            {
                /* 之前已经在触发中，检查是否达到时间阈值 */
                long long llTriggerDuration = llCurrentTime - triggerState.llTriggerStartTime;
                long long llTimeThreshold = rule.nTimeThreshold;

                /* 如果一直在连续触发，暂定连续触发 区域时间阈值加5秒后进行停止 */
                if (llTriggerDuration >= llTimeThreshold && llTriggerDuration < llTimeThreshold + 5)
                {
                    /* 达到时间阈值，触发真正的告警 */
                    dlog_info("物品拿取侦测-区域[%d] 连续触发 %lld 秒 >= %d 秒，灵敏度 %d <= %d，触发告警",
                              i,
                              llTriggerDuration,
                              rule.nTimeThreshold,
                              result.u32Sensitivity,
                              rule.nSensitivity);
                    bAnyRegionAlarm = true;

                    /* 请求主循环强制更新参考帧 */
                    bForceRefUpdate = true;
                    /* 强制更新时不需要延迟 */
                    bDelayRefUpdate = false;

#if CAP_EXHIBITION_OSD_PANEL
                    upsert_exhibition_panel_item(pstPanelFrame,
                                                 build_item_panel_item(static_cast<int>(i),
                                                                       rule.stRegion,
                                                                       result,
                                                                       true));
#endif

                    /* 只有事件开始时执行 */
                    if(!s_bOsdRegion)
                    {
                        /* 添加结果至动态分析数组 */
                        add_result_to_vector(rule.stRegion, vstRectInfo);
                        s_bOsdRegion = true;
                    }
                    /* 只要有一个区域达到告警条件，就可以停止遍历 */
                    break;
                }
                else if (llTriggerDuration >= llTimeThreshold + 5)
                {
                    if (!access("testPrint", F_OK))
                    {
                        dlog_debug(
                            "物品拿取侦测-区域[%d] 连续触发，持续了 %lld 秒 >（ 时间阈值 %d + 5 秒）,进行强制停止触发",
                            i,
                            llTriggerDuration,
                            rule.nTimeThreshold);
                    }

                    /* 请求主循环强制更新参考帧 */
                    bForceRefUpdate = true;
                    /* 强制更新时不需要延迟 */
                    bDelayRefUpdate = false;

                    /* 重置状态 */
                    triggerState.bTriggering = false;
                    triggerState.llTriggerStartTime = 0;
                    s_bOsdRegion = false;
                }
                else if (!access("testPrint", F_OK))
                {
                    dlog_debug("物拿取侦测-区域[%d] 持续触发中，已触发 %lld 秒 / %d 秒，灵敏度 %d <= %d",
                               i,
                               llTriggerDuration,
                               rule.nTimeThreshold,
                               result.u32Sensitivity,
                               rule.nSensitivity);
                }
            }
        }
        else
        {
            /* 当前帧不满足触发条件,触发中断 */
            if (triggerState.bTriggering)
            {
                /* 之前在触发中，现在中断了，重置状态 */
                long long llTriggerDuration = llCurrentTime - triggerState.llTriggerStartTime;
                if (!access("testPrint", F_OK))
                {
                    dlog_debug("物品拿取侦测-区域[%d] 触发中断，持续了 %lld 秒（ 时间阈值 %d 秒）",
                               i,
                               llTriggerDuration,
                               rule.nTimeThreshold);
                }
                triggerState.bTriggering = false;
                triggerState.llTriggerStartTime = 0;
                s_bOsdRegion = false;
            }
        }
    }

    /* 物品拿取告警状态机处理 */
    EventTriggerContext_S stAlarmCtx;
    stAlarmCtx.enEventType = Event::Type_E::OBJECT_REMOVAL;
    stAlarmCtx.nChnId = stCtx.nChnId;
    stAlarmCtx.llTimestamp = stCtx.llTimestamp;
#ifdef ENABLE_TVSDK_SRC
    if (bAnyRegionAlarm && stCtx.pFrameInfo != nullptr)
    {
        auto pPayload = std::make_shared<EventTvSdkPayload_S>();
        pPayload->enType = get_tvsdk_payload_type(stAlarmCtx.enEventType);
        if (AiAppCommon::encode_video_frame_to_jpeg_memory(stCtx.pFrameInfo, pPayload->stPanoramaImage) == OK)
        {
            stAlarmCtx.pTvSdkPayload = pPayload;
        }
    }
#endif
    auto state = m_objectRemovalAlarmStateMachine.handleAlarmState(bAnyRegionAlarm, stAlarmCtx);
    if(state)
    {
        /* 如果状态机处于活跃（报警中），一定要延迟更新 */
        bDelayRefUpdate = true;
    }

    return bDelayRefUpdate;
}

// info /*----------------------- 工具函数 -----------------------*/

template <typename T>
void CItemDetect::convertResolutionAndEnable(T &stConfig)
{
    if (!stConfig.aRule.empty())
    {
        /* 是否有任何一个区域初始化成功 */
        bool bIsInit = false;

        /* 转换区域坐标分辨率至算法分辨率 */
        for (auto &rule : stConfig.aRule) /* 使用引用而不是值拷贝 */
        {
            rule.stRegion.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight);
            /* 判断是否设置了正确的多边形 */
            if (rule.stRegion.IsValid())
            {
                bIsInit = true;
                /* 不要提前返回，继续转换其他区域 */
            }
        }

        /* 没有一个正确的多边形区域，不使能 */
        if (!bIsInit)
        {
            stConfig.bEnable = false;
        }
    }
}

#else
CItemDetect::CItemDetect()
    : m_dateQueue(QUEUE_MAX),
    m_unattendedObjectIndexManager("物品遗留"),
    m_objectRemovalIndexManager("物品拿取")
{
    /* 初始化检测状态 */
    for (size_t i = 0; i < 4; i++)
    {
        for (size_t j = 0; j < RESULT_NUM; j++)
        {
            /* 初始化物品遗留侦测状态 */
            m_stUnattendedObjectStatus[i][j].bIsInRegion = false;
            m_stUnattendedObjectStatus[i][j].dEnterTime = 0;
            m_stUnattendedObjectStatus[i][j].bAlarmed = false;

            /* 初始化物品拿取侦测状态 */
            m_stObjectRemovalStatus[i][j].bIsInRegion = false;
            m_stObjectRemovalStatus [i][j].dEnterTime = 0;
            m_stObjectRemovalStatus[i][j].bAlarmed = false;
        }
    }

    /* 启动线程 */
    m_bRunning.store(true);
    m_thread = std::thread(&CItemDetect::run, this);
}

CItemDetect::~CItemDetect()
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

void CItemDetect::recvMediaData(MediaData_S stMediaData)
{
    if (!m_stUnattendedObjectDetCfg.bEnable && !m_stObjectRemovalDetCfg.bEnable)
    {
        return;
    }

    if (m_RecvManager.handleEvent(stMediaData.stMediaParam.nChannel))
    {
        if (m_dateQueue.size() >= QUEUE_MAX)
        {
            dlog_error("物品侦测-数据队列满了 [%d]", m_dateQueue.size());
        }
        m_dateQueue.pushOrReplace(stMediaData);
    }
}

void CItemDetect::setAlgoEnCfg(const Event::AlgorithmConfig &stAlgoConfig)
{
    m_stUnattendedObjectDetCfg.bEnable = stAlgoConfig.nEnUnattendedObject;
    m_stObjectRemovalDetCfg.bEnable = stAlgoConfig.nEnObjectRemoval;

    if (m_stUnattendedObjectDetCfg.bEnable)
    {
        Alarm::UnattendedObject_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
    if (m_stObjectRemovalDetCfg.bEnable)
    {
        Alarm::ObjectRemoval_S stInfo;
        CEventConfigure::instance()->get_configure(stInfo);
        setAlgoParamCfg(stInfo);
    }
}

void CItemDetect::setAlgoParamCfg(const Alarm::UnattendedObject_S &stAlgoCfg)
{
    dlog_debug("ai_app: 设置物品遗留侦测参数");
    m_stUnattendedObjectDetCfg = stAlgoCfg;

    /* 转换区域坐标并判断有无正确的多边形区域，是否使能算法 */
    convertResolutionAndEnable(m_stUnattendedObjectDetCfg);
}

void CItemDetect::setAlgoParamCfg(const Alarm::ObjectRemoval_S &stAlgoCfg)
{
    dlog_debug("ai_app: 设置物品拿取侦测参数");
    m_stObjectRemovalDetCfg = stAlgoCfg;

    /* 转换区域坐标并判断有无正确的多边形区域，是否使能算法 */
    convertResolutionAndEnable(m_stObjectRemovalDetCfg);
}

bool CItemDetect::init()
{
    if (!m_pItemDetHandle)
    {
        m_pItemDetHandle = streamAiDetect_init(AI_DETECT_CHN_PACKAGE, AI_PACKAGE_NORMAL_MODEL_PATH);
        if (!m_pItemDetHandle)
        {
            dlog_error("物品侦测初始化失败");
            return false;
        }
        dlog_info("物品侦测初始化成功");
    }

    return true;
}

/* 反初始化 */
bool CItemDetect::unInit()
{
    if (m_pItemDetHandle)
    {
        streamAiDetect_uninit(m_pItemDetHandle);
    }

    return true;
}

bool CItemDetect::reboot()
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

void CItemDetect::run()
{
    pthread_setname_np(pthread_self(), "ItemDetect");

    /* 媒体信息 */
    MediaData_S stMediaData;

    while (m_bRunning.load())
    {
        if (!m_pItemDetHandle)
        {
            /* 没有算法使能，不进行算法初始化 */
            if (!m_stUnattendedObjectDetCfg.bEnable && !m_stObjectRemovalDetCfg.bEnable)
            {
                sleep(1);
                continue;
            }

            if (!init())
            {
                dlog_error("等待物品侦测初始化");
                /* 延迟等待 1s */
                std::unique_lock<std::mutex> lock(m_mutex);
                m_condition.wait_for(lock, std::chrono::seconds(1),
                                     [this]
                                     {
                                         return !m_bRunning.load();
                                     });
            }
            continue;
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

        /* 物品侦测 */
        if (m_pItemDetHandle->svpAiDetect_sendFrame(m_pItemDetHandle, &pFrameInfo->video_frame) == TD_SUCCESS)
        {
            /* OSD 动态分析显示数组 */
            std::vector<Common::RectInfo_S> vstRectInfo;
            /* 构造事件处理上下文 */
            SEventProcessContext stCtx;
            stCtx.nChnId = stMediaData.stMediaParam.nChannel;
            stCtx.llTimestamp = TimeUtils_NS::get_currentTimestampMs();
            stCtx.pFrameInfo = pFrameInfo;
            /* 打印输出数据 */
            m_pItemDetHandle->svpAiDetect_printResult(&m_pItemDetHandle->stResult);

            /* 物品遗留侦测 */
            if (m_stUnattendedObjectDetCfg.bEnable)
            {
                // double time = time_get_ms();
                /* 物品遗留侦测后处理函数 */
                processUnattendedObjectDetect(m_pItemDetHandle->stResult, vstRectInfo, stCtx);
                // dlog_debug("物品遗留侦测后处理函数耗时：%f", time_get_ms() - time);
            }
            /* 物品拿取侦测 */
            if(m_stObjectRemovalDetCfg.bEnable)
            {
                // double time = time_get_ms();
                /* 物品拿取侦测后处理函数 */
                processObjectRemovalDetect(m_pItemDetHandle->stResult, vstRectInfo, stCtx);
                // dlog_debug("物品拿取侦测后处理函数耗时：%f", time_get_ms() - time);
            }
            // note 调试用 动态分析显示
            if (m_stUnattendedObjectDetCfg.bEnable || m_stObjectRemovalDetCfg.bEnable) 
            {
                if (!access("testPrint", F_OK) && !vstRectInfo.empty())
                {
                    /* 发送结果至OSD模块，进行框选显示 */
                    send_detectionResult_to_osd(m_nWidth, m_nHeight, vstRectInfo);
                }
            }
        }
    }
}

// info /*----------------------- 算法后处理 -----------------------*/

void CItemDetect::processUnattendedObjectDetect(ot_aidetect_result_array &stResult,
                                                std::vector<Common::RectInfo_S> &vstRectInfo,
                                                const SEventProcessContext &stCtx)
{
    /* 遍历所有支持的目标类型并进行检测 */
    std::vector<ot_aidetect_class> supportedClasses = {
        OT_AIDETECT_CLASS_GARBAGE,  /* 垃圾 */
        OT_AIDETECT_CLASS_BAG,      /* 包裹 */
        OT_AIDETECT_CLASS_WALLET,   /* 钱包 */
        OT_AIDETECT_CLASS_PHONE     /* 手机 */
    };

    /* 是否有任何一个目标触发了报警 */
    bool bIsAlarmedOverall = false;

    for (ot_aidetect_class targetClass : supportedClasses)
    {
        /* 查找对应类型的检测结果 */
        ot_aidetect_object_of_one_class *pstObjectClass = nullptr;
        for (size_t i = 0; i < stResult.class_num; i++)
        {
            if (stResult.object_class[i].class_type == targetClass)
            {
                pstObjectClass = &stResult.object_class[i];
                break;
            }
        }

        /* 如果找到该类型的检测结果，调用通用处理函数 */
        bIsAlarmedOverall |= processUnattendedObjectRegionDetection(pstObjectClass, vstRectInfo);
    }

    /* 如果有任何一个目标触发了报警，则执行联动 */
    EventTriggerContext_S stAlarmCtx;
    stAlarmCtx.enEventType = Event::Type_E::UNATTENDED_OBJECT;
    stAlarmCtx.nChnId = stCtx.nChnId;
    stAlarmCtx.llTimestamp = stCtx.llTimestamp;
#ifdef ENABLE_TVSDK_SRC
    if (bIsAlarmedOverall && stCtx.pFrameInfo != nullptr)
    {
        auto pPayload = std::make_shared<EventTvSdkPayload_S>();
        pPayload->enType = get_tvsdk_payload_type(stAlarmCtx.enEventType);
        if (AiAppCommon::encode_video_frame_to_jpeg_memory(stCtx.pFrameInfo, pPayload->stPanoramaImage) == OK)
        {
            stAlarmCtx.pTvSdkPayload = pPayload;
        }
    }
#endif
    m_unattendedObjectAlarmStateMachine.handleAlarmState(bIsAlarmedOverall, stAlarmCtx);
}

void CItemDetect::processObjectRemovalDetect(ot_aidetect_result_array &stResult,
                                             std::vector<Common::RectInfo_S> &vstRectInfo,
                                             const SEventProcessContext &stCtx)
{
    /* 遍历所有支持的目标类型并进行检测 */
    std::vector<ot_aidetect_class> supportedClasses = {
        OT_AIDETECT_CLASS_GARBAGE,  /* 垃圾 */
        OT_AIDETECT_CLASS_BAG,      /* 包裹 */
        OT_AIDETECT_CLASS_WALLET,   /* 钱包 */
        OT_AIDETECT_CLASS_PHONE     /* 手机 */
    };

    /* 是否有任何一个目标触发了报警 */
    bool bIsAlarmedOverall = false;

    for (ot_aidetect_class targetClass : supportedClasses)
    {
        /* 查找对应类型的检测结果 */
        ot_aidetect_object_of_one_class *pstObjectClass = nullptr;
        for (size_t i = 0; i < stResult.class_num; i++)
        {
            if (stResult.object_class[i].class_type == targetClass)
            {
                pstObjectClass = &stResult.object_class[i];
                break;
            }
        }

        /* 调用物品拿取专用处理函数 */
        bIsAlarmedOverall |= processObjectRemovalRegionDetection(pstObjectClass, vstRectInfo);
    }

    /* 如果有任何一个目标触发了报警，则执行联动 */
    EventTriggerContext_S stAlarmCtx;
    stAlarmCtx.enEventType = Event::Type_E::OBJECT_REMOVAL;
    stAlarmCtx.nChnId = stCtx.nChnId;
    stAlarmCtx.llTimestamp = stCtx.llTimestamp;
#ifdef ENABLE_TVSDK_SRC
    if (bIsAlarmedOverall && stCtx.pFrameInfo != nullptr)
    {
        auto pPayload = std::make_shared<EventTvSdkPayload_S>();
        pPayload->enType = get_tvsdk_payload_type(stAlarmCtx.enEventType);
        if (AiAppCommon::encode_video_frame_to_jpeg_memory(stCtx.pFrameInfo, pPayload->stPanoramaImage) == OK)
        {
            stAlarmCtx.pTvSdkPayload = pPayload;
        }
    }
#endif
    m_objectRemovalAlarmStateMachine.handleAlarmState(bIsAlarmedOverall, stAlarmCtx);
}

// info /*----------------------- 工具函数 -----------------------*/

template <typename T>
void CItemDetect::convertResolutionAndEnable(T &stConfig)
{
    if (!stConfig.aRule.empty())
    {
        /* 是否有任何一个区域初始化成功 */
        bool bIsInit = false;

        /* 转换区域坐标分辨率至算法分辨率 */
        for (auto &rule : stConfig.aRule) /* 使用引用而不是值拷贝 */
        {
            rule.stRegion.ConvertResolution(PIXEL_WIDTH_1920, PIXEL_HEIGHT_1080, m_nWidth, m_nHeight);
            /* 判断是否设置了正确的多边形 */
            if (rule.stRegion.IsValid())
            {
                bIsInit = true;
                /* 不要提前返回，继续转换其他区域 */
            }
        }

        /* 没有一个正确的多边形区域，不使能 */
        if (!bIsInit)
        {
            stConfig.bEnable = false;
        }
    }
}

bool CItemDetect::processUnattendedObjectRegionDetection(const ot_aidetect_object_of_one_class *pstObjectClass, std::vector<Common::RectInfo_S> &vstRectInfo)
{
    if (!pstObjectClass)
    {
        return false;
    }

    /* 收集当前存在的track_id */
    std::set<int> current_track_ids;
    for (size_t i = 0; i < pstObjectClass->object_num; i++)
    {
        current_track_ids.insert(pstObjectClass->objects[i].track_id);
    }

    /* 清理已经消失的目标状态 */
    m_unattendedObjectIndexManager.cleanupLostTargets(current_track_ids);

    /* 是否有报警 */
    bool bIsAlarm = false;
    /* 当前时间戳 */
    double current_time = get_time_ms();

    /* 遍历检测到的目标 */
    for (size_t i = 0; i < pstObjectClass->object_num; i++)
    {
        const ot_aidetect_object &stObject = pstObjectClass->objects[i];
        int track_id = stObject.track_id;

        /* 获取或分配内部索引 */
        int internal_index = m_unattendedObjectIndexManager.getOrAllocateIndex(track_id);
        if (internal_index >= m_unattendedObjectIndexManager.getMaxTargets())
        {
            dlog_warn("无法为track_id %u 分配索引，跳过物品遗留处理", track_id);
            continue;
        }

        /* 遍历所有检测区域 */
        for (size_t j = 0; j < m_stUnattendedObjectDetCfg.aRule.size() && j < 4; j++)
        {
            const auto &stRule = m_stUnattendedObjectDetCfg.aRule[j];

            /* 检查灵敏度阈值 */
            float fSensitivityThreshold = 1.0f - stRule.nSensitivity / 100.0f;
            if (stObject.detect_confidence < fSensitivityThreshold)
            {
                continue; /* 置信度不够，跳过这个目标在当前区域的检测 */
            }

            /* 检查目标是否在当前区域内 */
            bool bInRegion = is_in_region(stRule.stRegion, stObject);
            AreaStatus_S &stAreaStatus = m_stUnattendedObjectStatus[j][internal_index];

            if (bInRegion && !stAreaStatus.bIsInRegion)
            {
                /* 目标刚进入区域 */
                stAreaStatus.bIsInRegion = true;
                stAreaStatus.dEnterTime = current_time;
                stAreaStatus.bAlarmed = false;
                dlog_debug("目标 ID: %u (内部索引: %d) 进入物品遗留区域 %zu", track_id, internal_index, j + 1);
            }
            else if (!bInRegion && stAreaStatus.bIsInRegion)
            {
                /* 目标离开区域 */
                stAreaStatus.bIsInRegion = false;
                stAreaStatus.dEnterTime = 0;
                stAreaStatus.bAlarmed = false;
                dlog_debug("目标 ID: %u (内部索引: %d) 离开物品遗留区域 %zu", track_id, internal_index, j + 1);
            }
            else if (bInRegion && stAreaStatus.bIsInRegion)
            {
                /* 目标持续在区域内，计算停留时间 */
                double stay_time_ms = current_time - stAreaStatus.dEnterTime;
                uint32_t stay_time_sec = (uint32_t) (stay_time_ms / 1000);

                /* 如果停留时间超过阈值且尚未报警，则触发报警 */
                if (stay_time_sec >= stRule.nTimeThreshold && !stAreaStatus.bAlarmed)
                {
                    stAreaStatus.bAlarmed = true;
                    bIsAlarm = true;

                    dlog_debug("目标 ID: %u (内部索引: %d) 在物品遗留区域 %zu 停留超时，触发报警", track_id, internal_index, j + 1);
                }
            }

            /* 添加到OSD显示 */
            if (bInRegion)
            {
                Common::RectInfo_S stInfo;
                stInfo.nX1 = stObject.detect_rect.x;
                stInfo.nY1 = stObject.detect_rect.y;
                stInfo.nX2 = stObject.detect_rect.width + stInfo.nX1;
                stInfo.nY2 = stObject.detect_rect.height + stInfo.nY1;
                vstRectInfo.emplace_back(stInfo);
            }
        }
    }

    return bIsAlarm;
}

bool CItemDetect::processObjectRemovalRegionDetection(const ot_aidetect_object_of_one_class *pstObjectClass, std::vector<Common::RectInfo_S> &vstRectInfo)
{
    /* 收集当前存在的track_id */
    std::set<int> current_track_ids;
    if (pstObjectClass)
    {
        for (size_t i = 0; i < pstObjectClass->object_num; i++)
        {
            current_track_ids.insert(pstObjectClass->objects[i].track_id);
        }
    }

    /* 获取之前存在的track_id */
    std::set<int> previous_track_ids = m_objectRemovalIndexManager.getCurrentTrackIds();

    /* 是否有报警 */
    bool bIsAlarm = false;
    /* 当前时间戳 */
    double current_time = get_time_ms();

    /* 检查是否有目标消失（物品被拿取） */
    for (int previous_track_id : previous_track_ids)
    {
        if (current_track_ids.find(previous_track_id) == current_track_ids.end())
        {
            /* 目标消失，检查是否在监控区域内 */
            int internal_index = m_objectRemovalIndexManager.getIndexByTrackId(previous_track_id);
            if (internal_index >= 0 && internal_index < m_objectRemovalIndexManager.getMaxTargets())
            {
                /* 遍历所有检测区域 */
                for (size_t j = 0; j < m_stObjectRemovalDetCfg.aRule.size() && j < 4; j++)
                {
                    AreaStatus_S &stAreaStatus = m_stObjectRemovalStatus[j][internal_index];

                    /* 如果目标之前在区域内且停留时间满足要求，触发拿取报警 */
                    if (stAreaStatus.bIsInRegion)
                    {
                        double stay_time_ms = current_time - stAreaStatus.dEnterTime;
                        uint32_t stay_time_sec = (uint32_t) (stay_time_ms / 1000);

                        /* 判断停留时间是否满足阈值 */
                        if (stay_time_sec >= m_stObjectRemovalDetCfg.aRule[j].nTimeThreshold)
                        {
                            bIsAlarm = true;
                            dlog_debug("目标 ID: %u (内部索引: %d) 从物品拿取区域 %zu 被拿取，触发报警", previous_track_id, internal_index, j + 1);
                        }

                        /* 清理状态 */
                        stAreaStatus.bIsInRegion = false;
                        stAreaStatus.dEnterTime = 0;
                        stAreaStatus.bAlarmed = false;
                    }
                }
            }
        }
    }

    /* 处理当前检测到的目标 */
    if (pstObjectClass)
    {
        for (size_t i = 0; i < pstObjectClass->object_num; i++)
        {
            const ot_aidetect_object &stObject = pstObjectClass->objects[i];
            int track_id = stObject.track_id;

            /* 获取或分配内部索引 */
            int internal_index = m_objectRemovalIndexManager.getOrAllocateIndex(track_id);
            if (internal_index >= m_objectRemovalIndexManager.getMaxTargets())
            {
                dlog_warn("无法为track_id %u 分配索引，跳过物品拿取处理", track_id);
                continue;
            }

            /* 遍历所有检测区域 */
            for (size_t j = 0; j < m_stObjectRemovalDetCfg.aRule.size() && j < 4; j++)
            {
                const auto &stRule = m_stObjectRemovalDetCfg.aRule[j];

                /* 检查灵敏度阈值 */
                float fSensitivityThreshold = 1.0f - stRule.nSensitivity / 100.0f;
                if (stObject.detect_confidence < fSensitivityThreshold)
                {
                    continue; /* 置信度不够，跳过这个目标在当前区域的检测 */
                }

                /* 检查目标是否在当前区域内 */
                bool bInRegion = is_in_region(stRule.stRegion, stObject);
                AreaStatus_S &stAreaStatus = m_stObjectRemovalStatus[j][internal_index];

                if (bInRegion && !stAreaStatus.bIsInRegion)
                {
                    /* 目标刚进入区域 */
                    stAreaStatus.bIsInRegion = true;
                    stAreaStatus.dEnterTime = current_time;
                    stAreaStatus.bAlarmed = false;
                    dlog_debug("目标 ID: %u (内部索引: %d) 进入物品拿取区域 %zu", track_id, internal_index, j + 1);
                }
                else if (!bInRegion && stAreaStatus.bIsInRegion)
                {
                    /* 目标离开区域 */
                    stAreaStatus.bIsInRegion = false;
                    stAreaStatus.dEnterTime = 0;
                    stAreaStatus.bAlarmed = false;
                    dlog_debug("目标 ID: %u (内部索引: %d) 离开物品拿取区域 %zu", track_id, internal_index, j + 1);
                }

                /* 添加到OSD显示 */
                if (bInRegion)
                {
                    Common::RectInfo_S stInfo;
                    stInfo.nX1 = stObject.detect_rect.x;
                    stInfo.nY1 = stObject.detect_rect.y;
                    stInfo.nX2 = stObject.detect_rect.width + stInfo.nX1;
                    stInfo.nY2 = stObject.detect_rect.height + stInfo.nY1;
                    vstRectInfo.emplace_back(stInfo);
                }
            }
        }
    }

    /* 清理已经消失的目标状态 */
    m_objectRemovalIndexManager.cleanupLostTargets(current_track_ids);

    return bIsAlarm;
}
#endif
